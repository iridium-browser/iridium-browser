// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/sdl_player_base.h"

#include <chrono>
#include <sstream>
#include <utility>

#include "cast/standalone_receiver/avcodec_glue.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/encoded_frame.h"
#include "util/big_endian.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace cast {

SDLPlayerBase::SDLPlayerBase(ClockNowFunctionPtr now_function,
                             TaskRunner* task_runner,
                             Receiver* receiver,
                             const std::string& codec_name,
                             std::function<void()> error_callback,
                             const char* media_type)
    : now_(now_function),
      receiver_(receiver),
      error_callback_(std::move(error_callback)),
      media_type_(media_type),
      decoder_(codec_name),
      decode_alarm_(now_, task_runner),
      render_alarm_(now_, task_runner),
      presentation_alarm_(now_, task_runner) {
  OSP_DCHECK(receiver_);
  OSP_DCHECK(media_type_);

  decoder_.set_client(this);
  receiver_->SetConsumer(this);
  ResumeRendering();
}

SDLPlayerBase::~SDLPlayerBase() {
  receiver_->SetConsumer(nullptr);
  decoder_.set_client(nullptr);
}

void SDLPlayerBase::OnFatalError(std::string message) {
  state_ = kError;
  error_status_ = Error(Error::Code::kUnknownError, std::move(message));

  // Halt decoding and clear the rendering queue.
  receiver_->SetConsumer(nullptr);
  decoder_.set_client(nullptr);
  decode_alarm_.Cancel();
  frames_to_render_.clear();

  // Resume rendering, to emit an error indication (e.g., "red splash" screen).
  ResumeRendering();

  if (error_callback_) {
    const auto callback = std::move(error_callback_);
    callback();
  }
}

Clock::time_point SDLPlayerBase::ResyncAndDeterminePresentationTime(
    const EncodedFrame& frame) {
  constexpr auto kMaxPlayoutDrift = milliseconds(100);
  const auto media_time_since_last_sync =
      (frame.rtp_timestamp - last_sync_rtp_timestamp_)
          .ToDuration<Clock::duration>(receiver_->rtp_timebase());
  Clock::time_point presentation_time =
      last_sync_reference_time_ + media_time_since_last_sync;
  const auto drift = to_milliseconds(frame.reference_time - presentation_time);
  if (drift > kMaxPlayoutDrift || drift < -kMaxPlayoutDrift) {
    // Only log if not the very first frame.
    OSP_LOG_IF(INFO, frame.frame_id != FrameId::first())
        << "Playout drift (" << drift.count() << " ms) exceeded threshold ("
        << kMaxPlayoutDrift.count() << " ms) for " << media_type_
        << ". Re-synchronizing...";
    // This is the "big-stick" way to re-synchronize. If the amount of drift
    // is small, a production-worthy player should "nudge" things gradually
    // back into sync over several frames.
    last_sync_rtp_timestamp_ = frame.rtp_timestamp;
    last_sync_reference_time_ = frame.reference_time;
    presentation_time = frame.reference_time;
  }
  return presentation_time;
}

void SDLPlayerBase::OnFramesReady(int buffer_size) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  // Do not consume anything if there are too many frames in the pipeline
  // already.
  if (static_cast<int>(frames_to_render_.size()) > kMaxFramesInPipeline) {
    return;
  }

  // Consume the next frame.
  const Clock::time_point start_time = now_();
  buffer_.Resize(buffer_size);
  EncodedFrame frame = receiver_->ConsumeNextFrame(buffer_.AsByteBuffer());

  // Create the tracking state for the frame in the player pipeline.
  OSP_DCHECK_EQ(frames_to_render_.count(frame.frame_id), 0);
  PendingFrame& pending_frame = frames_to_render_[frame.frame_id];
  pending_frame.start_time = start_time;

  pending_frame.presentation_time = ResyncAndDeterminePresentationTime(frame);

  // Start decoding the frame. This call may synchronously call back into the
  // AVCodecDecoder::Client methods in this class.
  decoder_.Decode(frame.frame_id, buffer_);
}

void SDLPlayerBase::OnFrameDecoded(FrameId frame_id, const AVFrame& frame) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  const auto it = frames_to_render_.find(frame_id);
  if (it == frames_to_render_.end()) {
    return;
  }
  OSP_DCHECK(!it->second.decoded_frame);
  // av_clone_frame() does a shallow copy here, incrementing a ref-count on the
  // memory backing the frame.
  it->second.decoded_frame = AVFrameUniquePtr(av_frame_clone(&frame));
  ResumeRendering();
}

void SDLPlayerBase::OnDecodeError(FrameId frame_id, std::string message) {
  const auto it = frames_to_render_.find(frame_id);
  if (it != frames_to_render_.end()) {
    frames_to_render_.erase(it);
  }
  OSP_LOG_WARN << "Requesting " << media_type_
               << " key frame because of error decoding" << frame_id << ": "
               << message;
  receiver_->RequestKeyFrame();
  ResumeDecoding();
}

void SDLPlayerBase::RenderAndSchedulePresentation() {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  // If something has already been scheduled to present at an exact time point,
  // don't render anything new yet.
  if (state_ == kScheduledToPresent) {
    return;
  }

  // If no frames are available, just re-render the currently-presented frame
  // (or the error screen).
  auto it =
      (state_ == kError) ? frames_to_render_.end() : frames_to_render_.begin();
  if (it == frames_to_render_.end() || !it->second.decoded_frame) {
    if (RenderWhileIdle(state_ == kPresented ? &current_frame_ : nullptr)) {
      // Schedule presentation to happen after a rather lengthy interval, to
      // minimize redraw/etc. resource usage while doing "idle mode" play-out.
      // The interval here, is "lengthy" from the program's perspective, but
      // reasonably "snappy" from the user's perspective.
      constexpr auto kIdlePresentInterval = milliseconds(250);
      presentation_alarm_.ScheduleFromNow(
          [this] {
            Present();
            ResumeRendering();
          },
          kIdlePresentInterval);
    }
    return;
  }

  // Skip late frames, to render the first not-late frame. If all decoded frames
  // are late, skip-forward to the least-late frame.
  const Clock::time_point now = now_();
  while (it->second.presentation_time < now) {
    const auto next_it = std::next(it);
    if (next_it == frames_to_render_.end() || !next_it->second.decoded_frame) {
      break;
    }
    frames_to_render_.erase(it);  // Drop the late frame.
    it = next_it;
  }

  // Remove the frame from the queue, making it the |current_frame_|. Then,
  // render it and, if successful, schedule its presentation.
  current_frame_ = std::move(it->second);
  frames_to_render_.erase(it);
  const ErrorOr<Clock::time_point> presentation_time =
      RenderNextFrame(current_frame_);
  if (!presentation_time) {
    OnFatalError(presentation_time.error().message());
    return;
  }
  state_ = kScheduledToPresent;
  presentation_alarm_.Schedule(
      [this] {
        Present();
        if (state_ == kScheduledToPresent) {
          state_ = kPresented;
        }
        ResumeRendering();
      },
      presentation_time.value());

  // Resume consuming/decoding frames, since some of the prior OnFramesReady()
  // calls may have been ignored to leave things in the Receiver's queue.
  ResumeDecoding();

  // Compute how long it took to decode/render this frame, and notify the
  // Receiver of the recent-average per-frame processing time. This is used by
  // the Receiver to determine when to drop late frames.
  const Clock::duration measured_processing_time =
      now_() - current_frame_.start_time;
  constexpr int kCumulativeAveragePoints = 8;
  recent_processing_time_ =
      ((kCumulativeAveragePoints - 1) * recent_processing_time_ +
       1 * measured_processing_time) /
      kCumulativeAveragePoints;
  receiver_->SetPlayerProcessingTime(recent_processing_time_);
}

void SDLPlayerBase::ResumeDecoding() {
  decode_alarm_.Schedule(
      [this] {
        const int buffer_size = receiver_->AdvanceToNextFrame();
        if (buffer_size != Receiver::kNoFramesReady) {
          OnFramesReady(buffer_size);
        }
      },
      Alarm::kImmediately);
}

void SDLPlayerBase::ResumeRendering() {
  render_alarm_.Schedule([this] { RenderAndSchedulePresentation(); },
                         Alarm::kImmediately);
}

// static
constexpr int SDLPlayerBase::kMaxFramesInPipeline;

SDLPlayerBase::PresentableFrame::PresentableFrame() = default;
SDLPlayerBase::PresentableFrame::~PresentableFrame() = default;
SDLPlayerBase::PresentableFrame::PresentableFrame(PresentableFrame&&) noexcept =
    default;
SDLPlayerBase::PresentableFrame& SDLPlayerBase::PresentableFrame::operator=(
    PresentableFrame&&) noexcept = default;

SDLPlayerBase::PendingFrame::PendingFrame() = default;
SDLPlayerBase::PendingFrame::~PendingFrame() = default;
SDLPlayerBase::PendingFrame::PendingFrame(PendingFrame&&) noexcept = default;
SDLPlayerBase::PendingFrame& SDLPlayerBase::PendingFrame::operator=(
    PendingFrame&&) noexcept = default;

}  // namespace cast
}  // namespace openscreen
