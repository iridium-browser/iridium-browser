// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/packet_receive_stats_tracker.h"

#include <chrono>
#include <limits>

#include "cast/streaming/constants.h"
#include "gtest/gtest.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {
namespace {

// Returns a RtcpReportBlock with all fields set to known values to see how the
// fields are modified by functions called during the tests.
RtcpReportBlock GetSentinel() {
  RtcpReportBlock report;
  report.ssrc = Ssrc{0x1337beef};
  report.packet_fraction_lost_numerator = -999;
  report.cumulative_packets_lost = -0x1337cafe;
  report.extended_high_sequence_number = 0x98765432;
  report.jitter =
      RtpTimeDelta::FromTicks(std::numeric_limits<int64_t>::max() - 42);
  report.last_status_report_id = StatusReportId{2222222222};
  report.delay_since_last_report = RtcpReportBlock::Delay(-0x3550641);
  return report;
}

// Run gtest expectations, that no fields were changed.
#define EXPECT_FIELDS_NOT_POPULATED(x)                                        \
  do {                                                                        \
    const RtcpReportBlock sentinel = GetSentinel();                           \
    EXPECT_EQ(sentinel.ssrc, (x).ssrc);                                       \
    EXPECT_EQ(sentinel.packet_fraction_lost_numerator,                        \
              (x).packet_fraction_lost_numerator);                            \
    EXPECT_EQ(sentinel.cumulative_packets_lost, (x).cumulative_packets_lost); \
    EXPECT_EQ(sentinel.extended_high_sequence_number,                         \
              (x).extended_high_sequence_number);                             \
    EXPECT_EQ(sentinel.jitter, (x).jitter);                                   \
    EXPECT_EQ(sentinel.last_status_report_id, (x).last_status_report_id);     \
    EXPECT_EQ(sentinel.delay_since_last_report, (x).delay_since_last_report); \
  } while (false)

// Run gtest expectations, that only the fields changed by
// PacketReceiveStatsTracker::PopulateNextReport() were changed.
#define EXPECT_FIELDS_POPULATED(x)                                            \
  do {                                                                        \
    const RtcpReportBlock sentinel = GetSentinel();                           \
    /* Fields that should remain untouched by PopulateNextReport(). */        \
    EXPECT_EQ(sentinel.ssrc, (x).ssrc);                                       \
    EXPECT_EQ(sentinel.last_status_report_id, (x).last_status_report_id);     \
    EXPECT_EQ(sentinel.delay_since_last_report, (x).delay_since_last_report); \
    /* Fields that should have changed.*/                                     \
    EXPECT_NE(sentinel.packet_fraction_lost_numerator,                        \
              (x).packet_fraction_lost_numerator);                            \
    EXPECT_NE(sentinel.cumulative_packets_lost, (x).cumulative_packets_lost); \
    EXPECT_NE(sentinel.extended_high_sequence_number,                         \
              (x).extended_high_sequence_number);                             \
    EXPECT_NE(sentinel.jitter, (x).jitter);                                   \
  } while (false)

TEST(PacketReceiveStatsTrackerTest, DoesNotPopulateReportWithoutData) {
  PacketReceiveStatsTracker tracker(kRtpVideoTimebase);
  RtcpReportBlock report = GetSentinel();
  tracker.PopulateNextReport(&report);
  EXPECT_FIELDS_NOT_POPULATED(report);
}

TEST(PacketReceiveStatsTrackerTest, PopulatesReportWithOnePacketTracked) {
  constexpr uint16_t kSequenceNumber = 1234;
  constexpr RtpTimeTicks kRtpTimestamp =
      RtpTimeTicks() + RtpTimeDelta::FromTicks(42);
  constexpr auto kArrivalTime = Clock::time_point() + seconds(3600);

  PacketReceiveStatsTracker tracker(kRtpVideoTimebase);
  tracker.OnReceivedValidRtpPacket(kSequenceNumber, kRtpTimestamp,
                                   kArrivalTime);

  RtcpReportBlock report = GetSentinel();
  tracker.PopulateNextReport(&report);
  EXPECT_FIELDS_POPULATED(report);
  EXPECT_EQ(0, report.packet_fraction_lost_numerator);
  EXPECT_EQ(0, report.cumulative_packets_lost);
  EXPECT_EQ(kSequenceNumber, report.extended_high_sequence_number);
  EXPECT_EQ(RtpTimeDelta(), report.jitter);
}

TEST(PacketReceiveStatsTrackerTest, WhenReceivingAllPackets) {
  // Set the first sequence number such that wraparound is going to be tested.
  constexpr uint16_t kFirstSequenceNumber =
      std::numeric_limits<uint16_t>::max() - 2;
  constexpr RtpTimeTicks kFirstRtpTimestamp =
      RtpTimeTicks() + RtpTimeDelta::FromTicks(42);
  constexpr auto kFirstArrivalTime = Clock::time_point() + seconds(3600);

  PacketReceiveStatsTracker tracker(kRtpVideoTimebase);

  // Record 10 packets arrived exactly one second apart with media timestamps
  // also exactly one second apart.
  for (int i = 0; i < 10; ++i) {
    tracker.OnReceivedValidRtpPacket(
        kFirstSequenceNumber + i,
        kFirstRtpTimestamp + RtpTimeDelta::FromTicks(kRtpVideoTimebase) * i,
        kFirstArrivalTime + seconds(i));
  }

  RtcpReportBlock report = GetSentinel();
  tracker.PopulateNextReport(&report);
  EXPECT_FIELDS_POPULATED(report);

  // Nothing should indicate to the tracker that any packets were dropped.
  EXPECT_EQ(0, report.packet_fraction_lost_numerator);
  EXPECT_EQ(0, report.cumulative_packets_lost);

  // The |extended_high_sequence_number| should reflect the wraparound of the
  // 16-bit counter value.
  EXPECT_EQ(uint32_t{65542}, report.extended_high_sequence_number);

  // There should be zero jitter, based on the timing information that was given
  // for each RTP packet.
  EXPECT_EQ(RtpTimeDelta(), report.jitter);
}

TEST(PacketReceiveStatsTrackerTest, WhenReceivingAboutHalfThePackets) {
  constexpr uint16_t kFirstSequenceNumber = 3;
  constexpr RtpTimeTicks kFirstRtpTimestamp =
      RtpTimeTicks() + RtpTimeDelta::FromTicks(99);
  constexpr auto kFirstArrivalTime = Clock::time_point() + seconds(8888);

  PacketReceiveStatsTracker tracker(kRtpVideoTimebase);

  // Record 10 packet arrivals whose sequence numbers step by 2, which should
  // indicate half of the packets didn't arrive.
  //
  // Ten arrived: 1, 3, 5, 7, 9, 11, 13, 15, 17, 19
  // Nine inferred missing: 2, 4, 6, 8, 10, 12, 14, 16, 18
  for (int i = 0; i < 10; ++i) {
    tracker.OnReceivedValidRtpPacket(
        kFirstSequenceNumber + (i * 2 + 1),
        kFirstRtpTimestamp + RtpTimeDelta::FromTicks(kRtpVideoTimebase) * i,
        kFirstArrivalTime + seconds(i));
  }

  RtcpReportBlock report = GetSentinel();
  tracker.PopulateNextReport(&report);
  EXPECT_FIELDS_POPULATED(report);
  EXPECT_EQ(121, report.packet_fraction_lost_numerator);
  EXPECT_EQ(9, report.cumulative_packets_lost);
  EXPECT_EQ(uint32_t{22}, report.extended_high_sequence_number);
  // There should be zero jitter, based on the timing information that was given
  // for each RTP packet.
  EXPECT_EQ(RtpTimeDelta(), report.jitter);
}

TEST(PacketReceiveStatsTrackerTest, ComputesJitterCorrectly) {
  constexpr uint16_t kFirstSequenceNumber = 3;
  constexpr RtpTimeTicks kFirstRtpTimestamp =
      RtpTimeTicks() + RtpTimeDelta::FromTicks(99);
  constexpr auto kFirstArrivalTime = Clock::time_point() + seconds(8888);

  // Record 100 packet arrivals, one second apart, where each packet's RTP
  // timestamps are progressing 2 seconds forward. Thus, the jitter calculation
  // should gradually converge towards a difference of one second.
  constexpr auto kTrueJitter = Clock::to_duration(seconds(1));
  PacketReceiveStatsTracker tracker(kRtpVideoTimebase);
  Clock::duration last_diff = Clock::duration::max();
  for (int i = 0; i < 100; ++i) {
    tracker.OnReceivedValidRtpPacket(
        kFirstSequenceNumber + i,
        kFirstRtpTimestamp +
            RtpTimeDelta::FromTicks(kRtpVideoTimebase) * (i * 2),
        kFirstArrivalTime + seconds(i));

    // Expect that the jitter is becoming closer to the actual value in each
    // iteration.
    RtcpReportBlock report;
    tracker.PopulateNextReport(&report);
    const auto diff = kTrueJitter - report.jitter.ToDuration<Clock::duration>(
                                        kRtpVideoTimebase);
    EXPECT_LT(diff, last_diff);
    last_diff = diff;
  }

  // Because the jitter calculation is a weighted moving average, and also
  // because the timebase has to be converted here, the metric might not ever
  // become exactly kTrueJitter. Ensure that it has converged reasonably close
  // to that value.
  RtcpReportBlock report;
  tracker.PopulateNextReport(&report);
  const auto diff = kTrueJitter - report.jitter.ToDuration<Clock::duration>(
                                      kRtpVideoTimebase);
  constexpr auto kMaxDiffAtEnd = Clock::to_duration(milliseconds(2));
  EXPECT_NEAR(0, static_cast<double>(diff.count()),
              static_cast<double>(kMaxDiffAtEnd.count()));
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
