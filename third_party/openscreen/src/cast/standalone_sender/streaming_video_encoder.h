// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_STREAMING_VIDEO_ENCODER_H_
#define CAST_STANDALONE_SENDER_STREAMING_VIDEO_ENCODER_H_

#include <algorithm>
#include <condition_variable>  // NOLINT
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtp_time.h"
#include "cast/streaming/sender.h"
#include "platform/api/task_runner.h"
#include "platform/api/time.h"

namespace openscreen {

class TaskRunner;

namespace cast {

class StreamingVideoEncoder {
 public:
  // Configurable parameters passed to the StreamingVpxEncoder constructor.
  struct Parameters {
    // Number of threads to parallelize frame encoding. This should be set based
    // on the number of CPU cores available for encoding, but no more than 8.
    int num_encode_threads =
        std::min(std::max<int>(std::thread::hardware_concurrency(), 1), 8);

    // Best-quality quantizer (lower is better quality). Range: [0,63]
    int min_quantizer = 4;

    // Worst-quality quantizer (lower is better quality). Range: [0,63]
    int max_quantizer = kMaxQuantizer;

    // Worst-quality quantizer to use when the CPU is extremely constrained.
    // Range: [min_quantizer,max_quantizer]
    int max_cpu_saver_quantizer = 25;

    // Maximum amount of wall-time a frame's encode can take, relative to the
    // frame's duration, before the CPU-saver logic is activated. The default
    // (70%) is appropriate for systems with four or more cores, but should be
    // reduced (e.g., 50%) for systems with fewer than three cores.
    //
    // Example: For 30 FPS (continuous) video, the frame duration is ~33.3ms,
    // and a value of 0.5 here would mean that the CPU-saver logic starts
    // sacrificing quality when frame encodes start taking longer than ~16.7ms.
    double max_time_utilization = 0.7;

    // Determines which codec (VP8, VP9, or AV1) is to be used for encoding.
    // Defaults to VP8.
    VideoCodec codec = VideoCodec::kVp8;
  };

  // Represents an input VideoFrame, passed to EncodeAndSend().
  struct VideoFrame {
    // Image width and height.
    int width = 0;
    int height = 0;

    // I420 format image pointers and row strides (the number of bytes between
    // the start of successive rows). The pointers only need to remain valid
    // until the EncodeAndSend() call returns.
    const uint8_t* yuv_planes[3] = {};
    int yuv_strides[3] = {};

    // How long this frame will be held before the next frame will be displayed,
    // or zero if unknown. The frame duration is passed to the video codec,
    // affecting a number of important behaviors, including: per-frame
    // bandwidth, CPU time spent encoding, temporal quality trade-offs, and
    // key/golden/alt-ref frame generation intervals.
    Clock::duration duration;
  };

  // Performance statistics for a single frame's encode.
  //
  // For full details on how to use these stats in an end-to-end system, see:
  // https://www.chromium.org/developers/design-documents/
  //     auto-throttled-screen-capture-and-mirroring
  // and https://source.chromium.org/chromium/chromium/src/+/master:
  //     media/cast/sender/performance_metrics_overlay.h
  struct Stats {
    // The Cast Streaming ID that was assigned to the frame.
    FrameId frame_id;

    // The RTP timestamp of the frame.
    RtpTimeTicks rtp_timestamp;

    // How long the frame took to encode. This is wall time, not CPU time or
    // some other load metric.
    Clock::duration encode_wall_time;

    // The frame's predicted duration; or, the actual duration if it was
    // provided in the VideoFrame.
    Clock::duration frame_duration;

    // The encoded frame's size in bytes.
    int encoded_size = 0;

    // The average size of an encoded frame in bytes, having this
    // |frame_duration| and current target bitrate.
    double target_size = 0.0;

    // The actual quantizer the video encoder used, in the range [0,63].
    int quantizer = 0;

    // The "hindsight" quantizer value that would have produced the best quality
    // encoding of the frame at the current target bitrate. The nominal range is
    // [0.0,63.0]. If it is larger than 63.0, then it was impossible to
    // encode the frame within the current target bitrate (e.g., too much
    // "entropy" in the image, or too low a target bitrate).
    double perfect_quantizer = 0.0;

    // Utilization feedback metrics. The nominal range for each of these is
    // [0.0,1.0] where 1.0 means "the entire budget available for the frame was
    // exhausted." Going above 1.0 is okay for one or a few frames, since it's
    // the average over many frames that matters before the system is considered
    // "redlining."
    //
    // The max of these three provides an overall utilization control signal.
    // The usual approach is for upstream control logic to increase/decrease the
    // data volume (e.g., video resolution and/or frame rate) to maintain a good
    // target point.
    double time_utilization() const {
      return static_cast<double>(encode_wall_time.count()) /
             static_cast<double>(frame_duration.count());
    }
    double space_utilization() const { return encoded_size / target_size; }
    double entropy_utilization() const {
      return perfect_quantizer / kMaxQuantizer;
    }
  };

  virtual ~StreamingVideoEncoder();

  // Get/Set the target bitrate. This may be changed at any time, as frequently
  // as desired, and it will take effect internally as soon as possible.
  virtual int GetTargetBitrate() const = 0;
  virtual void SetTargetBitrate(int new_bitrate) = 0;

  // Encode |frame| using the video encoder, assemble an EncodedFrame, and
  // enqueue into the Sender. The frame may be dropped if too many frames are
  // in-flight. If provided, the |stats_callback| is run after the frame is
  // enqueued in the Sender (via the main TaskRunner).
  virtual void EncodeAndSend(const VideoFrame& frame,
                             Clock::time_point reference_time,
                             std::function<void(Stats)> stats_callback) = 0;

  static constexpr int kMinQuantizer = 0;
  static constexpr int kMaxQuantizer = 63;

 protected:
  StreamingVideoEncoder(const Parameters& params,
                        TaskRunner* task_runner,
                        std::unique_ptr<Sender> sender);

  // This is the equivalent change in encoding speed per one quantizer step.
  static constexpr double kEquivalentEncodingSpeedStepPerQuantizerStep =
      1 / 20.0;

  // Updates the |ideal_speed_setting_|, to take effect with the next frame
  // encode, based on the given performance |stats|.
  void UpdateSpeedSettingForNextFrame(const Stats& stats);

  const Parameters params_;
  TaskRunner* const main_task_runner_;
  std::unique_ptr<Sender> sender_;

  // These represent the magnitude of the AV1 speed setting, where larger values
  // (i.e., faster speed) request less CPU usage but will provide lower video
  // quality. Only the encode thread accesses these.
  double ideal_speed_setting_;  // A time-weighted average, from measurements.
  int current_speed_setting_;   // Current |encoder_| speed setting.

  // This member should be last in the class since the thread should not start
  // until all above members have been initialized by the constructor.
  std::thread encode_thread_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_STREAMING_VIDEO_ENCODER_H_
