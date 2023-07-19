// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/bandwidth_estimator.h"

#include <algorithm>

#include "util/osp_logging.h"
#include "util/saturate_cast.h"

namespace openscreen {
namespace cast {

using clock_operators::operator<<;

namespace {

// Converts units from |bytes| per |time_window| number of Clock ticks into
// bits-per-second.
int ToClampedBitsPerSecond(int32_t bytes, Clock::duration time_window) {
  OSP_DCHECK_GT(time_window, Clock::duration::zero());

  // Divide |bytes| by |time_window| and scale the units to bits per second.
  constexpr int64_t kBitsPerByte = 8;
  constexpr int64_t kClockTicksPerSecond =
      Clock::to_duration(std::chrono::seconds(1)).count();
  const int64_t bits = bytes * kBitsPerByte;
  const int64_t bits_per_second =
      (bits * kClockTicksPerSecond) / time_window.count();
  return saturate_cast<int>(bits_per_second);
}

}  // namespace

BandwidthEstimator::BandwidthEstimator(int max_packets_per_timeslice,
                                       Clock::duration timeslice_duration,
                                       Clock::time_point start_time)
    : max_packets_per_history_window_(max_packets_per_timeslice *
                                      kNumTimeslices),
      history_window_(timeslice_duration * kNumTimeslices),
      burst_history_(timeslice_duration, start_time),
      feedback_history_(timeslice_duration, start_time) {
  OSP_DCHECK_GT(max_packets_per_timeslice, 0);
  OSP_DCHECK_GT(timeslice_duration, Clock::duration::zero());
}

BandwidthEstimator::~BandwidthEstimator() = default;

void BandwidthEstimator::OnBurstComplete(int num_packets_sent,
                                         Clock::time_point when) {
  OSP_DCHECK_GE(num_packets_sent, 0);
  burst_history_.Accumulate(num_packets_sent, when);
}

void BandwidthEstimator::OnRtcpReceived(
    Clock::time_point arrival_time,
    Clock::duration estimated_round_trip_time) {
  OSP_DCHECK_GE(estimated_round_trip_time, Clock::duration::zero());
  // Move forward the feedback history tracking timeline to include the latest
  // moment a packet could have left the Sender.
  feedback_history_.AdvanceToIncludeTime(arrival_time -
                                         estimated_round_trip_time);
}

void BandwidthEstimator::OnPayloadReceived(
    int payload_bytes_acknowledged,
    Clock::time_point ack_arrival_time,
    Clock::duration estimated_round_trip_time) {
  OSP_DCHECK_GE(payload_bytes_acknowledged, 0);
  OSP_DCHECK_GE(estimated_round_trip_time, Clock::duration::zero());
  // Track the bytes in terms of when the last packet was sent.
  feedback_history_.Accumulate(payload_bytes_acknowledged,
                               ack_arrival_time - estimated_round_trip_time);
}

int BandwidthEstimator::ComputeNetworkBandwidth() const {
  // Determine whether the |burst_history_| time window overlaps with the
  // |feedback_history_| time window by at least half. The time windows don't
  // have to overlap entirely because the calculations are averaging all the
  // measurements (i.e., recent typical behavior). Though, they should overlap
  // by "enough" so that the measurements correlate "enough."
  const Clock::time_point overlap_begin =
      std::max(burst_history_.begin_time(), feedback_history_.begin_time());
  const Clock::time_point overlap_end =
      std::min(burst_history_.end_time(), feedback_history_.end_time());
  if ((overlap_end - overlap_begin) < (history_window_ / 2)) {
    return 0;
  }

  const int32_t num_packets_transmitted = burst_history_.Sum();
  if (num_packets_transmitted <= 0) {
    // Cannot estimate because there have been no transmissions recently.
    return 0;
  }
  const Clock::duration transmit_duration = history_window_ *
                                            num_packets_transmitted /
                                            max_packets_per_history_window_;
  const int32_t num_bytes_received = feedback_history_.Sum();
  return ToClampedBitsPerSecond(num_bytes_received, transmit_duration);
}

// static
constexpr int BandwidthEstimator::kNumTimeslices;

BandwidthEstimator::FlowTracker::FlowTracker(Clock::duration timeslice_duration,
                                             Clock::time_point begin_time)
    : timeslice_duration_(timeslice_duration), begin_time_(begin_time) {}

BandwidthEstimator::FlowTracker::~FlowTracker() = default;

void BandwidthEstimator::FlowTracker::AdvanceToIncludeTime(
    Clock::time_point until) {
  if (until < end_time()) {
    return;  // Not advancing.
  }

  // Step forward in time, at timeslice granularity.
  const int64_t num_periods = 1 + (until - end_time()) / timeslice_duration_;
  begin_time_ += num_periods * timeslice_duration_;

  // Shift the ring elements, discarding N oldest timeslices, and creating N new
  // ones initialized to zero.
  const int shift_count = std::min<int64_t>(num_periods, kNumTimeslices);
  for (int i = 0; i < shift_count; ++i) {
    history_ring_[tail_++] = 0;
  }
}

void BandwidthEstimator::FlowTracker::Accumulate(int32_t amount,
                                                 Clock::time_point when) {
  if (when < begin_time_) {
    return;  // Ignore a data point that is already too old.
  }

  AdvanceToIncludeTime(when);

  // Because of the AdvanceToIncludeTime() call just made, the offset/index
  // calculations here are guaranteed to point to a valid element in the
  // |history_ring_|.
  const int64_t offset_from_first = (when - begin_time_) / timeslice_duration_;
  const index_mod_256_t ring_index = tail_ + offset_from_first;
  int32_t& timeslice = history_ring_[ring_index];
  timeslice = saturate_cast<int32_t>(int64_t{timeslice} + amount);
}

int32_t BandwidthEstimator::FlowTracker::Sum() const {
  int64_t result = 0;
  for (int32_t amount : history_ring_) {
    result += amount;
  }
  return saturate_cast<int32_t>(result);
}

}  // namespace cast
}  // namespace openscreen
