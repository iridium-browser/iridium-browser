// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/packet_receive_stats_tracker.h"

#include <algorithm>

namespace openscreen {
namespace cast {

PacketReceiveStatsTracker::PacketReceiveStatsTracker(int rtp_timebase)
    : rtp_timebase_(rtp_timebase) {}

PacketReceiveStatsTracker::~PacketReceiveStatsTracker() = default;

void PacketReceiveStatsTracker::OnReceivedValidRtpPacket(
    uint16_t sequence_number,
    RtpTimeTicks rtp_timestamp,
    Clock::time_point arrival_time) {
  if (num_rtp_packets_received_ == 0) {
    // Since this is the very first packet received, initialize all other
    // tracking stats.
    num_rtp_packets_received_at_last_report_ = 0;
    greatest_sequence_number_ = PacketSequenceNumber(sequence_number);
    base_sequence_number_ = greatest_sequence_number_.previous();
    greatest_sequence_number_at_last_report_ = base_sequence_number_;
    jitter_ = Clock::duration::zero();
  } else {
    // Update the greatest sequence number ever seen.
    const auto expanded_sequence_number =
        greatest_sequence_number_.Expand(sequence_number);
    if (expanded_sequence_number > greatest_sequence_number_) {
      greatest_sequence_number_ = expanded_sequence_number;
    }

    // Update the interarrival jitter. This is similar to the calculation in
    // Appendix A of the RFC 3550 spec (for RTP).
    const Clock::duration time_between_arrivals =
        arrival_time - last_rtp_packet_arrival_time_;
    const auto media_time_difference =
        (rtp_timestamp - last_rtp_packet_timestamp_)
            .ToDuration<Clock::duration>(rtp_timebase_);
    const auto delta = time_between_arrivals - media_time_difference;
    const auto absolute_delta =
        (delta < decltype(delta)::zero()) ? -delta : delta;
    jitter_ += (absolute_delta - jitter_) / 16;
  }

  ++num_rtp_packets_received_;
  last_rtp_packet_arrival_time_ = arrival_time;
  last_rtp_packet_timestamp_ = rtp_timestamp;
}

void PacketReceiveStatsTracker::PopulateNextReport(RtcpReportBlock* report) {
  if (num_rtp_packets_received_ <= 0) {
    // None of the packet loss, etc., tracking has valid values yet; so don't
    // populate anything.
    return;
  }

  report->SetPacketFractionLostNumerator(
      greatest_sequence_number_ - greatest_sequence_number_at_last_report_,
      num_rtp_packets_received_ - num_rtp_packets_received_at_last_report_);
  greatest_sequence_number_at_last_report_ = greatest_sequence_number_;
  num_rtp_packets_received_at_last_report_ = num_rtp_packets_received_;

  report->SetCumulativePacketsLost(
      greatest_sequence_number_ - base_sequence_number_,
      num_rtp_packets_received_);

  report->extended_high_sequence_number =
      greatest_sequence_number_.lower_32_bits();

  report->jitter = RtpTimeDelta::FromDuration(jitter_, rtp_timebase_);
}

}  // namespace cast
}  // namespace openscreen
