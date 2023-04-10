// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_sender/streaming_av1_encoder.h"

#include <aom/aomcx.h>

#include <chrono>
#include <cmath>
#include <utility>

#include "cast/standalone_sender/streaming_encoder_util.h"
#include "cast/streaming/encoded_frame.h"
#include "cast/streaming/environment.h"
#include "cast/streaming/sender.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"
#include "util/saturate_cast.h"

namespace openscreen {
namespace cast {

using clock_operators::operator<<;

namespace {

constexpr int kBytesPerKilobyte = 1024;

// Lower and upper bounds to the frame duration passed to aom_codec_encode(), to
// ensure sanity. Note that the upper-bound is especially important in cases
// where the video paused for some lengthy amount of time.
constexpr Clock::duration kMinFrameDuration = milliseconds(1);
constexpr Clock::duration kMaxFrameDuration = milliseconds(125);

// Highest/lowest allowed encoding speed set to the encoder.
constexpr int kHighestEncodingSpeed = 9;
constexpr int kLowestEncodingSpeed = 0;

}  // namespace

StreamingAv1Encoder::StreamingAv1Encoder(const Parameters& params,
                                         TaskRunner* task_runner,
                                         std::unique_ptr<Sender> sender)
    : StreamingVideoEncoder(params, task_runner, std::move(sender)) {
  ideal_speed_setting_ = kHighestEncodingSpeed;
  encode_thread_ = std::thread([this] { ProcessWorkUnitsUntilTimeToQuit(); });

  OSP_DCHECK(params_.codec == VideoCodec::kAv1);
  const auto result = aom_codec_enc_config_default(aom_codec_av1_cx(), &config_,
                                                   AOM_USAGE_REALTIME);
  OSP_CHECK_EQ(result, AOM_CODEC_OK);

  // This is set to non-zero in ConfigureForNewFrameSize() later, to flag that
  // the encoder has been initialized.
  config_.g_threads = 0;

  // Set the timebase to match that of openscreen::Clock::duration.
  config_.g_timebase.num = Clock::duration::period::num;
  config_.g_timebase.den = Clock::duration::period::den;

  // |g_pass| and |g_lag_in_frames| must be "one pass" and zero, respectively,
  // because of the way the libaom API is used.
  config_.g_pass = AOM_RC_ONE_PASS;
  config_.g_lag_in_frames = 0;

  // Rate control settings.
  config_.rc_dropframe_thresh = 0;  // The encoder may not drop any frames.
  config_.rc_resize_mode = 0;
  config_.rc_end_usage = AOM_CBR;
  config_.rc_target_bitrate = target_bitrate_ / kBytesPerKilobyte;
  config_.rc_min_quantizer = params_.min_quantizer;
  config_.rc_max_quantizer = params_.max_quantizer;

  // The reasons for the values chosen here (rc_*shoot_pct and rc_buf_*_sz) are
  // lost in history. They were brought-over from the legacy Chrome Cast
  // Streaming Sender implemenation.
  config_.rc_undershoot_pct = 100;
  config_.rc_overshoot_pct = 15;
  config_.rc_buf_initial_sz = 500;
  config_.rc_buf_optimal_sz = 600;
  config_.rc_buf_sz = 1000;

  config_.kf_mode = AOM_KF_DISABLED;
}

StreamingAv1Encoder::~StreamingAv1Encoder() {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    target_bitrate_ = 0;
    cv_.notify_one();
  }
  encode_thread_.join();
}

int StreamingAv1Encoder::GetTargetBitrate() const {
  // Note: No need to lock the |mutex_| since this method should be called on
  // the same thread as SetTargetBitrate().
  return target_bitrate_;
}

void StreamingAv1Encoder::SetTargetBitrate(int new_bitrate) {
  // Ensure that, when bps is converted to kbps downstream, that the encoder
  // bitrate will not be zero.
  new_bitrate = std::max(new_bitrate, kBytesPerKilobyte);

  std::unique_lock<std::mutex> lock(mutex_);
  // Only assign the new target bitrate if |target_bitrate_| has not yet been
  // used to signal the |encode_thread_| to end.
  if (target_bitrate_ > 0) {
    target_bitrate_ = new_bitrate;
  }
}

void StreamingAv1Encoder::EncodeAndSend(
    const VideoFrame& frame,
    Clock::time_point reference_time,
    std::function<void(Stats)> stats_callback) {
  WorkUnit work_unit;

  // TODO(jophba): The |VideoFrame| struct should provide the media timestamp,
  // instead of this code inferring it from the reference timestamps, since: 1)
  // the video capturer's clock may tick at a different rate than the system
  // clock; and 2) to reduce jitter.
  if (start_time_ == Clock::time_point::min()) {
    start_time_ = reference_time;
    work_unit.rtp_timestamp = RtpTimeTicks();
  } else {
    work_unit.rtp_timestamp = RtpTimeTicks::FromTimeSinceOrigin(
        reference_time - start_time_, sender_->rtp_timebase());
    if (work_unit.rtp_timestamp <= last_enqueued_rtp_timestamp_) {
      OSP_LOG_WARN << "VIDEO[" << sender_->ssrc()
                   << "] Dropping: RTP timestamp is not monotonically "
                      "increasing from last frame.";
      return;
    }
  }
  if (sender_->GetInFlightMediaDuration(work_unit.rtp_timestamp) >
      sender_->GetMaxInFlightMediaDuration()) {
    OSP_LOG_WARN << "VIDEO[" << sender_->ssrc()
                 << "] Dropping: In-flight media duration would be too high.";
    return;
  }

  Clock::duration frame_duration = frame.duration;
  if (frame_duration <= Clock::duration::zero()) {
    // The caller did not provide the frame duration in |frame|.
    if (reference_time == start_time_) {
      // Use the max for the first frame so libaom will spend extra effort on
      // its quality.
      frame_duration = kMaxFrameDuration;
    } else {
      // Use the actual amount of time between the current and previous frame as
      // a prediction for the next frame's duration.
      frame_duration =
          (work_unit.rtp_timestamp - last_enqueued_rtp_timestamp_)
              .ToDuration<Clock::duration>(sender_->rtp_timebase());
    }
  }
  work_unit.duration =
      std::max(std::min(frame_duration, kMaxFrameDuration), kMinFrameDuration);

  last_enqueued_rtp_timestamp_ = work_unit.rtp_timestamp;

  work_unit.image = CloneAsAv1Image(frame);
  work_unit.reference_time = reference_time;
  work_unit.stats_callback = std::move(stats_callback);
  const bool force_key_frame = sender_->NeedsKeyFrame();
  {
    std::unique_lock<std::mutex> lock(mutex_);
    needs_key_frame_ |= force_key_frame;
    encode_queue_.push(std::move(work_unit));
    cv_.notify_one();
  }
}

void StreamingAv1Encoder::DestroyEncoder() {
  OSP_DCHECK_EQ(std::this_thread::get_id(), encode_thread_.get_id());

  if (is_encoder_initialized()) {
    aom_codec_destroy(&encoder_);
    // Flag that the encoder is not initialized. See header comments for
    // is_encoder_initialized().
    config_.g_threads = 0;
  }
}

void StreamingAv1Encoder::ProcessWorkUnitsUntilTimeToQuit() {
  OSP_DCHECK_EQ(std::this_thread::get_id(), encode_thread_.get_id());

  for (;;) {
    WorkUnitWithResults work_unit{};
    bool force_key_frame;
    int target_bitrate;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (target_bitrate_ <= 0) {
        break;  // Time to end this thread.
      }
      if (encode_queue_.empty()) {
        cv_.wait(lock);
        if (encode_queue_.empty()) {
          continue;
        }
      }
      static_cast<WorkUnit&>(work_unit) = std::move(encode_queue_.front());
      encode_queue_.pop();
      force_key_frame = needs_key_frame_;
      needs_key_frame_ = false;
      target_bitrate = target_bitrate_;
    }

    // Clock::now() is being called directly, instead of using a
    // dependency-injected "now function," since actual wall time is being
    // measured.
    const Clock::time_point encode_start_time = Clock::now();
    PrepareEncoder(work_unit.image->d_w, work_unit.image->d_h, target_bitrate);
    EncodeFrame(force_key_frame, work_unit);
    ComputeFrameEncodeStats(Clock::now() - encode_start_time, target_bitrate,
                            work_unit);
    UpdateSpeedSettingForNextFrame(work_unit.stats);

    main_task_runner_->PostTask(
        [this, results = std::move(work_unit)]() mutable {
          SendEncodedFrame(std::move(results));
        });
  }

  DestroyEncoder();
}

void StreamingAv1Encoder::PrepareEncoder(int width,
                                         int height,
                                         int target_bitrate) {
  OSP_DCHECK_EQ(std::this_thread::get_id(), encode_thread_.get_id());

  const int target_kbps = target_bitrate / kBytesPerKilobyte;

  // Translate the |ideal_speed_setting_| into the AOME_SET_CPUUSED setting and
  // the minimum quantizer to use.
  int speed;
  int min_quantizer;
  if (ideal_speed_setting_ > kHighestEncodingSpeed) {
    speed = kHighestEncodingSpeed;
    const double remainder = ideal_speed_setting_ - speed;
    min_quantizer = rounded_saturate_cast<int>(
        remainder / kEquivalentEncodingSpeedStepPerQuantizerStep +
        params_.min_quantizer);
    min_quantizer = std::min(min_quantizer, params_.max_cpu_saver_quantizer);
  } else {
    speed = std::max(rounded_saturate_cast<int>(ideal_speed_setting_),
                     kLowestEncodingSpeed);
    min_quantizer = params_.min_quantizer;
  }

  if (static_cast<int>(config_.g_w) != width ||
      static_cast<int>(config_.g_h) != height) {
    DestroyEncoder();
  }

  if (!is_encoder_initialized()) {
    config_.g_threads = params_.num_encode_threads;
    config_.g_w = width;
    config_.g_h = height;
    config_.rc_target_bitrate = target_kbps;
    config_.rc_min_quantizer = min_quantizer;

    encoder_ = {};
    const aom_codec_flags_t flags = 0;

    const auto init_result =
        aom_codec_enc_init(&encoder_, aom_codec_av1_cx(), &config_, flags);
    OSP_CHECK_EQ(init_result, AOM_CODEC_OK);

    // Raise the threshold for considering macroblocks as static. The default is
    // zero, so this setting makes the encoder less sensitive to motion. This
    // lowers the probability of needing to utilize more CPU to search for
    // motion vectors.
    const auto ctl_result =
        aom_codec_control(&encoder_, AOME_SET_STATIC_THRESHOLD, 1);
    OSP_CHECK_EQ(ctl_result, AOM_CODEC_OK);

    // Ensure the speed will be set (below).
    current_speed_setting_ = ~speed;
  } else if (static_cast<int>(config_.rc_target_bitrate) != target_kbps ||
             static_cast<int>(config_.rc_min_quantizer) != min_quantizer) {
    config_.rc_target_bitrate = target_kbps;
    config_.rc_min_quantizer = min_quantizer;
    const auto update_config_result =
        aom_codec_enc_config_set(&encoder_, &config_);
    OSP_CHECK_EQ(update_config_result, AOM_CODEC_OK);
  }

  if (current_speed_setting_ != speed) {
    const auto ctl_result =
        aom_codec_control(&encoder_, AOME_SET_CPUUSED, speed);
    OSP_CHECK_EQ(ctl_result, AOM_CODEC_OK);
    current_speed_setting_ = speed;
  }
}

void StreamingAv1Encoder::EncodeFrame(bool force_key_frame,
                                      WorkUnitWithResults& work_unit) {
  OSP_DCHECK_EQ(std::this_thread::get_id(), encode_thread_.get_id());

  // The presentation timestamp argument here is fixed to zero to force the
  // encoder to base its single-frame bandwidth calculations entirely on
  // |frame_duration| and the target bitrate setting.
  const aom_codec_pts_t pts = 0;
  const aom_enc_frame_flags_t flags = force_key_frame ? AOM_EFLAG_FORCE_KF : 0;
  const auto encode_result = aom_codec_encode(
      &encoder_, work_unit.image.get(), pts, work_unit.duration.count(), flags);
  OSP_CHECK_EQ(encode_result, AOM_CODEC_OK);

  const aom_codec_cx_pkt_t* pkt;
  for (aom_codec_iter_t iter = nullptr;;) {
    pkt = aom_codec_get_cx_data(&encoder_, &iter);
    // aom_codec_get_cx_data() returns null once the "iteration" is complete.
    // However, that point should never be reached because a
    // AOM_CODEC_CX_FRAME_PKT must be encountered before that.
    OSP_CHECK(pkt);
    if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
      break;
    }
  }

  // A copy of the payload data is being made here. That's okay since it has to
  // be copied at some point anyway, to be passed back to the main thread.
  auto* const begin = static_cast<const uint8_t*>(pkt->data.frame.buf);
  auto* const end = begin + pkt->data.frame.sz;
  work_unit.payload.assign(begin, end);
  work_unit.is_key_frame = !!(pkt->data.frame.flags & AOM_FRAME_IS_KEY);
}

void StreamingAv1Encoder::ComputeFrameEncodeStats(
    Clock::duration encode_wall_time,
    int target_bitrate,
    WorkUnitWithResults& work_unit) {
  OSP_DCHECK_EQ(std::this_thread::get_id(), encode_thread_.get_id());

  Stats& stats = work_unit.stats;

  // Note: stats.frame_id is set later, in SendEncodedFrame().
  stats.rtp_timestamp = work_unit.rtp_timestamp;
  stats.encode_wall_time = encode_wall_time;
  stats.frame_duration = work_unit.duration;
  stats.encoded_size = work_unit.payload.size();

  constexpr double kBytesPerBit = 1.0 / CHAR_BIT;
  constexpr double kSecondsPerClockTick =
      1.0 / Clock::to_duration(seconds(1)).count();
  const double target_bytes_per_clock_tick =
      target_bitrate * (kBytesPerBit * kSecondsPerClockTick);
  stats.target_size = target_bytes_per_clock_tick * work_unit.duration.count();

  // The quantizer the encoder used. This is the result of the AV1 encoder
  // taking a guess at what quantizer value would produce an encoded frame size
  // as close to the target as possible.
  const auto get_quantizer_result = aom_codec_control(
      &encoder_, AOME_GET_LAST_QUANTIZER_64, &stats.quantizer);
  OSP_CHECK_EQ(get_quantizer_result, AOM_CODEC_OK);

  // Now that the frame has been encoded and the number of bytes is known, the
  // perfect quantizer value (i.e., the one that should have been used) can be
  // determined.
  stats.perfect_quantizer = stats.quantizer * stats.space_utilization();
}

void StreamingAv1Encoder::SendEncodedFrame(WorkUnitWithResults results) {
  OSP_DCHECK(main_task_runner_->IsRunningOnTaskRunner());

  EncodedFrame frame;
  frame.frame_id = sender_->GetNextFrameId();
  if (results.is_key_frame) {
    frame.dependency = EncodedFrame::Dependency::kKeyFrame;
    frame.referenced_frame_id = frame.frame_id;
  } else {
    frame.dependency = EncodedFrame::Dependency::kDependent;
    frame.referenced_frame_id = frame.frame_id - 1;
  }
  frame.rtp_timestamp = results.rtp_timestamp;
  frame.reference_time = results.reference_time;
  frame.data = ByteBuffer(results.payload);

  if (sender_->EnqueueFrame(frame) != Sender::OK) {
    // Since the frame will not be sent, the encoder's frame dependency chain
    // has been broken. Force a key frame for the next frame.
    std::unique_lock<std::mutex> lock(mutex_);
    needs_key_frame_ = true;
  }

  if (results.stats_callback) {
    results.stats.frame_id = frame.frame_id;
    results.stats_callback(results.stats);
  }
}

// static
StreamingAv1Encoder::Av1ImageUniquePtr StreamingAv1Encoder::CloneAsAv1Image(
    const VideoFrame& frame) {
  OSP_DCHECK_GE(frame.width, 0);
  OSP_DCHECK_GE(frame.height, 0);
  OSP_DCHECK_GE(frame.yuv_strides[0], 0);
  OSP_DCHECK_GE(frame.yuv_strides[1], 0);
  OSP_DCHECK_GE(frame.yuv_strides[2], 0);

  constexpr int kAlignment = 32;
  Av1ImageUniquePtr image(aom_img_alloc(nullptr, AOM_IMG_FMT_I420, frame.width,
                                        frame.height, kAlignment));
  OSP_CHECK(image);

  CopyPlane(frame.yuv_planes[0], frame.yuv_strides[0], frame.height,
            image->planes[AOM_PLANE_Y], image->stride[AOM_PLANE_Y]);
  CopyPlane(frame.yuv_planes[1], frame.yuv_strides[1], (frame.height + 1) / 2,
            image->planes[AOM_PLANE_U], image->stride[AOM_PLANE_U]);
  CopyPlane(frame.yuv_planes[2], frame.yuv_strides[2], (frame.height + 1) / 2,
            image->planes[AOM_PLANE_V], image->stride[AOM_PLANE_V]);

  return image;
}

}  // namespace cast
}  // namespace openscreen
