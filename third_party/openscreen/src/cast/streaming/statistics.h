// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_STATISTICS_H_
#define CAST_STREAMING_STATISTICS_H_

#include <stddef.h>
#include <stdint.h>

#include <utility>
#include <vector>

#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtp_time.h"
#include "platform/api/time.h"
#include "util/enum_name_table.h"

namespace openscreen {
namespace cast {

enum class StatisticType {
  // Frame enqueuing rate.
  kEnqueueFps = 0,

  // TODO(https://issuetracker.google.com/285417419): providable through this
  // API? support TBD.
  // Average capture latency in milliseconds.
  kAvgCaptureLatencyMs,

  // Average encode duration in milliseconds.
  kAvgEncodeTimeMs,

  // Duration from when a frame is encoded to when the packet is first
  // sent.
  kAvgQueueingLatencyMs,

  // Duration from when a packet is transmitted to when it is received.
  // This measures latency from sender to receiver.
  kAvgNetworkLatencyMs,

  // Duration from when a frame is encoded to when the packet is first
  // received.
  kAvgPacketLatencyMs,

  // Average latency between frame encoded and the moment when the frame
  // is fully received.
  kAvgFrameLatencyMs,

  // Duration from when a frame is captured to when it should be played out.
  kAvgEndToEndLatencyMs,

  // Encode bitrate in kbps.
  kEncodeRateKbps,

  // Packet transmission bitrate in kbps.
  kPacketTransmissionRateKbps,

  // Duration in milliseconds since last receiver response.
  kTimeSinceLastReceiverResponseMs,

  // Number of frames captured.
  kNumFramesCaptured,

  // Number of frames dropped by encoder.
  kNumFramesDroppedByEncoder,

  // Number of late frames.
  kNumLateFrames,

  // Number of packets that were sent.
  kNumPacketsSent,

  // Number of packets that were received by receiver.
  kNumPacketsReceived,

  // Unix time in milliseconds of first event since reset.
  kFirstEventTimeMs,

  // Unix time in milliseconds of last event since reset.
  kLastEventTimeMs,

  // The number of statistic types.
  kNumTypes = kLastEventTimeMs + 1
};

enum class HistogramType {
  // Histogram representing the capture latency (in milliseconds).
  kCaptureLatencyMs,

  // Histogram representing the encode time (in milliseconds).
  kEncodeTimeMs,

  // Histogram representing the queueing latency (in milliseconds).
  kQueueingLatencyMs,

  // Histogram representing the network latency (in milliseconds).
  kNetworkLatencyMs,

  // Histogram representing the packet latency (in milliseconds).
  kPacketLatencyMs,

  // Histogram representing the end to end latency (in milliseconds).
  kEndToEndLatencyMs,

  // Histogram representing how late frames are (in milliseconds).
  kFrameLatenessMs,

  // The number of histogram types.
  kNumTypes = kFrameLatenessMs + 1
};

struct SimpleHistogram {
  // This will create N+2 buckets where N = (max - min) / width:
  // Underflow bucket: < min
  // Bucket 0: [min, min + width - 1]
  // Bucket 1: [min + width, min + 2 * width - 1]
  // ...
  // Bucket N-1: [max - width, max - 1]
  // Overflow bucket: >= max
  // |min| must be less than |max|.
  // |width| must divide |max - min| evenly.

  SimpleHistogram();
  SimpleHistogram(int64_t min, int64_t max, int64_t width);
  SimpleHistogram(const SimpleHistogram&) = delete;
  SimpleHistogram(SimpleHistogram&&) noexcept;
  SimpleHistogram& operator=(const SimpleHistogram&) = delete;
  SimpleHistogram& operator=(SimpleHistogram&&);
  ~SimpleHistogram();

  void Add(int64_t sample);
  void Reset();

  int64_t min = 1;
  int64_t max = 1;
  int64_t width = 1;
  std::vector<int> buckets;
};

struct SenderStats {
  using StatisticsList =
      std::array<double, static_cast<size_t>(StatisticType::kNumTypes)>;
  using HistogramsList =
      std::array<SimpleHistogram,
                 static_cast<size_t>(HistogramType::kNumTypes)>;

  // The current audio statistics.
  StatisticsList audio_statistics;

  // The current audio histograms.
  HistogramsList audio_histograms;

  // The current video statistics.
  StatisticsList video_statistics;

  // The current video histograms.
  HistogramsList video_histograms;
};

// The consumer may provide a statistics client if they are interested in
// getting statistics about the ongoing session.
class SenderStatsClient {
  // Gets called regularly with updated statistics while they are being
  // generated.
  virtual void OnStatisticsUpdated(const SenderStats& updated_stats) = 0;

 protected:
  virtual ~SenderStatsClient();
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_STATISTICS_H_
