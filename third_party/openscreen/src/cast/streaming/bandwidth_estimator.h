// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_BANDWIDTH_ESTIMATOR_H_
#define CAST_STREAMING_BANDWIDTH_ESTIMATOR_H_

#include <stdint.h>

#include <limits>

#include "platform/api/time.h"

namespace openscreen {
namespace cast {

// Tracks send attempts and successful receives, and then computes a total
// network bandwith estimate.
//
// Two metrics are tracked by the BandwidthEstimator, over a "recent history"
// time window:
//
//   1. The number of packets sent during bursts (see SenderPacketRouter for
//      explanation of what a "burst" is). These track when the network was
//      actually in-use for transmission and the magnitude of each burst. When
//      computing bandwidth, the estimator assumes the timeslices where the
//      network was not in-use could have been used to send even more bytes at
//      the same rate.
//
//   2. Successful receipt of payload bytes over time, or a lack thereof.
//      Packets that include acknowledgements from the Receivers are providing
//      proof of the successful receipt of payload bytes. All other packets
//      provide proof of network connectivity over time, and are used to
//      identify periods of time where nothing was received.
//
// The BandwidthEstimator assumes a simplified model for streaming over the
// network. The model does not include any detailed knowledge about things like
// protocol overhead, packet re-transmits, parasitic bufferring, network
// reliability, etc. Instead, it automatically accounts for all such things by
// looking at what's actually leaving the Senders and what's actually making it
// to the Receivers.
//
// This simplified model does produce some known inaccuracies in the resulting
// estimations. If no data has recently been transmitted (or been received),
// estimations cannot be provided. If the transmission rate is near (or
// exceeding) the network's capacity, the estimations will be very accurate. In
// between those two extremes, the logic will tend to under-estimate the
// network's capacity. However, those under-estimates will still be far larger
// than the current transmission rate.
//
// Thus, these estimates can be used effectively as a control signal for
// congestion control in upstream code modules. The logic computing the media's
// encoding target bitrate should be adjusted in realtime using a TCP-like
// congestion control algorithm:
//
//   1. When the estimated bitrate is less than the current encoding target
//      bitrate, aggressively and immediately decrease the encoding bitrate.
//
//   2. When the estimated bitrate is more than the current encoding target
//      bitrate, gradually increase the encoding bitrate (up to the maximum
//      that is reasonable for the application).
class BandwidthEstimator {
 public:
  // |max_packets_per_timeslice| and |timeslice_duration| should match the burst
  // configuration in SenderPacketRouter. |start_time| should be a recent
  // point-in-time before the first packet is sent.
  BandwidthEstimator(int max_packets_per_timeslice,
                     Clock::duration timeslice_duration,
                     Clock::time_point start_time);

  ~BandwidthEstimator();

  // Returns the duration of the fixed, recent-history time window over which
  // data flows are being tracked.
  Clock::duration history_window() const { return history_window_; }

  // Records |when| burst-sending was active or inactive. For the active case,
  // |num_packets_sent| should include all network packets sent, including
  // non-payload packets (since both affect the modeled utilization/capacity).
  // For the inactive case, this method should be called with zero for
  // |num_packets_sent|.
  void OnBurstComplete(int num_packets_sent, Clock::time_point when);

  // Records when a RTCP packet was received. It's important for Senders to call
  // this any time a packet comes in from the Receivers, even if no payload is
  // being acknowledged, since the time windows of "nothing successfully
  // received" is also important information to track.
  void OnRtcpReceived(Clock::time_point arrival_time,
                      Clock::duration estimated_round_trip_time);

  // Records that some number of payload bytes has been acknowledged (i.e.,
  // successfully received).
  void OnPayloadReceived(int payload_bytes_acknowledged,
                         Clock::time_point ack_arrival_time,
                         Clock::duration estimated_round_trip_time);

  // Computes the current network bandwith estimate. Returns 0 if this cannot be
  // determined due to a lack of sufficiently-recent data.
  int ComputeNetworkBandwidth() const;

 private:
  // FlowTracker (below) manages a ring buffer of size 256. It simplifies the
  // index calculations to use an integer data type where all arithmetic is mod
  // 256.
  using index_mod_256_t = uint8_t;
  static constexpr int kNumTimeslices =
      static_cast<int>(std::numeric_limits<index_mod_256_t>::max()) + 1;

  // Tracks volume (e.g., the total number of payload bytes) over a fixed
  // recent-history time window. The time window is divided up into a number of
  // identical timeslices, each of which represents the total number of bytes
  // that flowed during a certain period of time. The data is accumulated in
  // ring buffer elements so that old data points drop-off as newer ones (that
  // move the history window forward) are added.
  class FlowTracker {
   public:
    FlowTracker(Clock::duration timeslice_duration,
                Clock::time_point begin_time);
    ~FlowTracker();

    Clock::time_point begin_time() const { return begin_time_; }
    Clock::time_point end_time() const {
      return begin_time_ + timeslice_duration_ * kNumTimeslices;
    }

    // Advance the end of the time window being tracked such that the
    // most-recent timeslice includes |until|. Too-old timeslices are dropped
    // and new ones are initialized to a zero amount.
    void AdvanceToIncludeTime(Clock::time_point until);

    // Accumulate the given |amount| into the timeslice that includes |when|.
    void Accumulate(int32_t amount, Clock::time_point when);

    // Return the sum of all the amounts in recent history. This clamps to the
    // valid range of int32_t, if necessary.
    int32_t Sum() const;

   private:
    const Clock::duration timeslice_duration_;

    // The beginning of the oldest timeslice in the recent-history time window,
    // the one pointed to by |tail_|.
    Clock::time_point begin_time_;

    // A ring buffer tracking the accumulated amount for each timeslice.
    int32_t history_ring_[kNumTimeslices]{};

    // The index of the oldest timeslice in the |history_ring_|. This can also
    // be thought of, equivalently, as the index just after the most-recent
    // timeslice.
    index_mod_256_t tail_ = 0;
  };

  // The maximum number of packet sends that could possibly be attempted during
  // the recent-history time window.
  const int max_packets_per_history_window_;

  // The range of time being tracked.
  const Clock::duration history_window_;

  // History tracking for send attempts, and success feeback. These timeseries
  // are in terms of when packets have left the Senders.
  FlowTracker burst_history_;
  FlowTracker feedback_history_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_BANDWIDTH_ESTIMATOR_H_
