// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/rtcp_common.h"

#include <chrono>
#include <limits>

#include "absl/types/span.h"
#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {
namespace {

template <typename T>
void SerializeAndExpectPointerAdvanced(const T& source,
                                       int num_bytes,
                                       uint8_t* buffer) {
  absl::Span<uint8_t> buffer_span(buffer, num_bytes);
  source.AppendFields(&buffer_span);
  EXPECT_EQ(buffer + num_bytes, buffer_span.data());
}

// Tests that the RTCP Common Header for a packet type that includes an Item
// Count is successfully serialized and re-parsed.
TEST(RtcpCommonTest, SerializesAndParsesHeaderForSenderReports) {
  RtcpCommonHeader original;
  original.packet_type = RtcpPacketType::kSenderReport;
  original.with.report_count = 31;
  original.payload_size = 16;

  uint8_t buffer[kRtcpCommonHeaderSize];
  SerializeAndExpectPointerAdvanced(original, kRtcpCommonHeaderSize, buffer);

  const auto parsed = RtcpCommonHeader::Parse(buffer);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(original.packet_type, parsed->packet_type);
  EXPECT_EQ(original.with.report_count, parsed->with.report_count);
  EXPECT_EQ(original.payload_size, parsed->payload_size);
}

// Tests that the RTCP Common Header for a packet type that includes a RTCP
// Subtype is successfully serialized and re-parsed.
TEST(RtcpCommonTest, SerializesAndParsesHeaderForCastFeedback) {
  RtcpCommonHeader original;
  original.packet_type = RtcpPacketType::kPayloadSpecific;
  original.with.subtype = RtcpSubtype::kFeedback;
  original.payload_size = 99 * sizeof(uint32_t);

  uint8_t buffer[kRtcpCommonHeaderSize];
  SerializeAndExpectPointerAdvanced(original, kRtcpCommonHeaderSize, buffer);

  const auto parsed = RtcpCommonHeader::Parse(buffer);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(original.packet_type, parsed->packet_type);
  EXPECT_EQ(original.with.subtype, parsed->with.subtype);
  EXPECT_EQ(original.payload_size, parsed->payload_size);
}

// Tests that a RTCP Common Header will not be parsed from an empty buffer.
TEST(RtcpCommonTest, WillNotParseHeaderFromEmptyBuffer) {
  const uint8_t kEmptyPacket[] = {};
  EXPECT_FALSE(
      RtcpCommonHeader::Parse(absl::Span<const uint8_t>(kEmptyPacket, 0))
          .has_value());
}

// Tests that a RTCP Common Header will not be parsed from a buffer containing
// garbage data.
TEST(RtcpCommonTest, WillNotParseHeaderFromGarbage) {
  // clang-format off
  const uint8_t kGarbage[] = {
    0x4f, 0x27, 0xeb, 0x22, 0x27, 0xeb, 0x22, 0x4f,
    0xeb, 0x22, 0x4f, 0x27, 0x22, 0x4f, 0x27, 0xeb,
  };
  // clang-format on
  EXPECT_FALSE(RtcpCommonHeader::Parse(kGarbage).has_value());
}

// Tests whether RTCP Common Header validation logic is correct.
TEST(RtcpCommonTest, WillNotParseHeaderWithInvalidData) {
  // clang-format off
  const uint8_t kCastFeedbackPacket[] = {
    0b10000001,  // Version=2, Padding=no, ItemCount=1 byte.
    206,  // RTCP Packet type byte.
    0x00, 0x04,  // Length of remainder of packet, in 32-bit words.
    9, 8, 7, 6,  // SSRC of receiver.
    1, 2, 3, 4,  // SSRC of sender.
    'C', 'A', 'S', 'T',
    0x0a,  // Checkpoint Frame ID (lower 8 bits).
    0x00,  // Number of "Loss Fields"
    0x00, 0x28,  // Current Playout Delay in milliseconds.
  };
  // clang-format on

  // Start with a valid packet, and expect the parse to succeed.
  uint8_t buffer[sizeof(kCastFeedbackPacket)];
  memcpy(buffer, kCastFeedbackPacket, sizeof(buffer));
  EXPECT_TRUE(RtcpCommonHeader::Parse(buffer).has_value());

  // Wrong version in first byte: Expect parse failure.
  buffer[0] = 0b01000001;
  EXPECT_FALSE(RtcpCommonHeader::Parse(buffer).has_value());
  buffer[0] = kCastFeedbackPacket[0];

  // Wrong packet type (not in RTCP range): Expect parse failure.
  buffer[1] = 42;
  EXPECT_FALSE(RtcpCommonHeader::Parse(buffer).has_value());
  buffer[1] = kCastFeedbackPacket[1];
}

// Test that the Report Block optionally included in Sender Reports or Receiver
// Reports can be serialized and re-parsed correctly.
TEST(RtcpCommonTest, SerializesAndParsesRtcpReportBlocks) {
  constexpr Ssrc kSsrc{0x04050607};

  RtcpReportBlock original;
  original.ssrc = kSsrc;
  original.packet_fraction_lost_numerator = 0x67;
  original.cumulative_packets_lost = 74536;
  original.extended_high_sequence_number = 0x0201fedc;
  original.jitter = RtpTimeDelta::FromTicks(123);
  original.last_status_report_id = 0x0908;
  original.delay_since_last_report = RtcpReportBlock::Delay(99999);

  uint8_t buffer[kRtcpReportBlockSize];
  SerializeAndExpectPointerAdvanced(original, kRtcpReportBlockSize, buffer);

  // If the number of report blocks is zero, or some other SSRC is specified,
  // ParseOne() should not return a result.
  EXPECT_FALSE(RtcpReportBlock::ParseOne(buffer, 0, 0).has_value());
  EXPECT_FALSE(RtcpReportBlock::ParseOne(buffer, 0, kSsrc).has_value());
  EXPECT_FALSE(RtcpReportBlock::ParseOne(buffer, 1, 0).has_value());

  // Expect that the report block is parsed correctly.
  const auto parsed = RtcpReportBlock::ParseOne(buffer, 1, kSsrc);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(original.ssrc, parsed->ssrc);
  EXPECT_EQ(original.packet_fraction_lost_numerator,
            parsed->packet_fraction_lost_numerator);
  EXPECT_EQ(original.cumulative_packets_lost, parsed->cumulative_packets_lost);
  EXPECT_EQ(original.extended_high_sequence_number,
            parsed->extended_high_sequence_number);
  EXPECT_EQ(original.jitter, parsed->jitter);
  EXPECT_EQ(original.last_status_report_id, parsed->last_status_report_id);
  EXPECT_EQ(original.delay_since_last_report, parsed->delay_since_last_report);
}

// Tests that the Report Block parser can, among multiple Report Blocks, find
// the one with a matching recipient SSRC.
TEST(RtcpCommonTest, ParsesOneReportBlockFromMultipleBlocks) {
  constexpr Ssrc kSsrc{0x04050607};
  constexpr int kNumBlocks = 5;

  RtcpReportBlock expected;
  expected.ssrc = kSsrc;
  expected.packet_fraction_lost_numerator = 0x67;
  expected.cumulative_packets_lost = 74536;
  expected.extended_high_sequence_number = 0x0201fedc;
  expected.jitter = RtpTimeDelta::FromTicks(123);
  expected.last_status_report_id = 0x0908;
  expected.delay_since_last_report = RtcpReportBlock::Delay(99999);

  // Generate multiple report blocks with different recipient SSRCs.
  uint8_t buffer[kRtcpReportBlockSize * kNumBlocks];
  absl::Span<uint8_t> buffer_span(buffer, kRtcpReportBlockSize * kNumBlocks);
  for (int i = 0; i < kNumBlocks; ++i) {
    RtcpReportBlock another;
    another.ssrc = expected.ssrc + i - 2;
    another.packet_fraction_lost_numerator =
        expected.packet_fraction_lost_numerator + i - 2;
    another.cumulative_packets_lost = expected.cumulative_packets_lost + i - 2;
    another.extended_high_sequence_number =
        expected.extended_high_sequence_number + i - 2;
    another.jitter = expected.jitter + RtpTimeDelta::FromTicks(i - 2);
    another.last_status_report_id = expected.last_status_report_id + i - 2;
    another.delay_since_last_report =
        expected.delay_since_last_report + RtcpReportBlock::Delay(i - 2);

    another.AppendFields(&buffer_span);
  }

  // Expect that the desired report block is found and parsed correctly.
  const auto parsed = RtcpReportBlock::ParseOne(buffer, kNumBlocks, kSsrc);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(expected.ssrc, parsed->ssrc);
  EXPECT_EQ(expected.packet_fraction_lost_numerator,
            parsed->packet_fraction_lost_numerator);
  EXPECT_EQ(expected.cumulative_packets_lost, parsed->cumulative_packets_lost);
  EXPECT_EQ(expected.extended_high_sequence_number,
            parsed->extended_high_sequence_number);
  EXPECT_EQ(expected.jitter, parsed->jitter);
  EXPECT_EQ(expected.last_status_report_id, parsed->last_status_report_id);
  EXPECT_EQ(expected.delay_since_last_report, parsed->delay_since_last_report);
}

// Tests the helper for computing the packet fraction loss numerator, a value
// that should always be between 0 and 255, in terms of absolute packet counts.
TEST(RtcpCommonTest, ComputesPacketLossFractionForReportBlocks) {
  const auto ComputeFractionLost = [](int64_t num_apparently_sent,
                                      int64_t num_received) {
    RtcpReportBlock report;
    report.SetPacketFractionLostNumerator(num_apparently_sent, num_received);
    return report.packet_fraction_lost_numerator;
  };

  // If no non-duplicate packets were sent to the Receiver, the packet loss
  // fraction should be zero.
  EXPECT_EQ(0, ComputeFractionLost(0, 0));
  EXPECT_EQ(0, ComputeFractionLost(0, 1));
  EXPECT_EQ(0, ComputeFractionLost(0, 999));

  // If the same number or more packets were received than those apparently
  // sent, the packet loss fraction should be zero.
  EXPECT_EQ(0, ComputeFractionLost(1, 1));
  EXPECT_EQ(0, ComputeFractionLost(1, 2));
  EXPECT_EQ(0, ComputeFractionLost(1, 4));
  EXPECT_EQ(0, ComputeFractionLost(4, 5));
  EXPECT_EQ(0, ComputeFractionLost(42, 42));
  EXPECT_EQ(0, ComputeFractionLost(60, 999));

  // Test various partial loss scenarios.
  EXPECT_EQ(85, ComputeFractionLost(3, 2));
  EXPECT_EQ(128, ComputeFractionLost(10, 5));
  EXPECT_EQ(174, ComputeFractionLost(22, 7));

  // Test various total-loss/near-total-loss scenarios.
  EXPECT_EQ(255, ComputeFractionLost(17, 0));
  EXPECT_EQ(255, ComputeFractionLost(100, 0));
  EXPECT_EQ(255, ComputeFractionLost(9876, 1));
}

// Tests the helper for computing the cumulative packet loss total, a value that
// should always be between 0 and 2^24 - 1, in terms of absolute packet counts.
TEST(RtcpCommonTest, ComputesCumulativePacketLossForReportBlocks) {
  const auto ComputeLoss = [](int64_t num_apparently_sent,
                              int64_t num_received) {
    RtcpReportBlock report;
    report.SetCumulativePacketsLost(num_apparently_sent, num_received);
    return report.cumulative_packets_lost;
  };

  // Test various no-loss scenarios (including duplicate packets).
  EXPECT_EQ(0, ComputeLoss(0, 0));
  EXPECT_EQ(0, ComputeLoss(0, 1));
  EXPECT_EQ(0, ComputeLoss(3, 3));
  EXPECT_EQ(0, ComputeLoss(56, 56));
  EXPECT_EQ(0, ComputeLoss(std::numeric_limits<int64_t>::max() - 12,
                           std::numeric_limits<int64_t>::max()));
  EXPECT_EQ(0, ComputeLoss(std::numeric_limits<int64_t>::max(),
                           std::numeric_limits<int64_t>::max()));

  // Test various partial loss scenarios.
  EXPECT_EQ(1, ComputeLoss(2, 1));
  EXPECT_EQ(2, ComputeLoss(42, 40));
  EXPECT_EQ(1025, ComputeLoss(999999, 999999 - 1025));
  EXPECT_EQ(1, ComputeLoss(std::numeric_limits<int64_t>::max(),
                           std::numeric_limits<int64_t>::max() - 1));

  // Test that a huge cumulative loss saturates to the maximum valid value for
  // the field.
  EXPECT_EQ((1 << 24) - 1, ComputeLoss(999999999, 1));
}

// Tests the helper that converts Clock::durations to the report blocks timebase
// (1/65536 sconds), and also that it saturates to to the valid range of values
// (0 to 2^32 - 1 ticks).
TEST(RtcpCommonTest, ComputesDelayForReportBlocks) {
  using Delay = RtcpReportBlock::Delay;

  const auto ComputeDelay = [](Clock::duration delay_in_wrong_timebase) {
    RtcpReportBlock report;
    report.SetDelaySinceLastReport(delay_in_wrong_timebase);
    return report.delay_since_last_report;
  };

  // A duration less than or equal to zero should clamp to zero.
  EXPECT_EQ(Delay::zero(), ComputeDelay(Clock::duration::min()));
  EXPECT_EQ(Delay::zero(), ComputeDelay(milliseconds{-1234}));
  EXPECT_EQ(Delay::zero(), ComputeDelay(Clock::duration::zero()));

  // Test conversion of various durations that should not clamp.
  EXPECT_EQ(Delay(32768 /* 1/2 second worth of ticks */),
            ComputeDelay(milliseconds(500)));
  EXPECT_EQ(Delay(65536 /* 1 second worth of ticks */),
            ComputeDelay(seconds(1)));
  EXPECT_EQ(Delay(655360 /* 10 seconds worth of ticks */),
            ComputeDelay(seconds(10)));
  EXPECT_EQ(Delay(4294967294), ComputeDelay(microseconds(65535999983)));
  EXPECT_EQ(Delay(4294967294), ComputeDelay(microseconds(65535999984)));

  // A too-large duration should clamp to the maximum-possible Delay value.
  EXPECT_EQ(Delay(4294967295), ComputeDelay(microseconds(65535999985)));
  EXPECT_EQ(Delay(4294967295), ComputeDelay(microseconds(65535999986)));
  EXPECT_EQ(Delay(4294967295), ComputeDelay(microseconds(999999000000)));
  EXPECT_EQ(Delay(4294967295), ComputeDelay(Clock::duration::max()));
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
