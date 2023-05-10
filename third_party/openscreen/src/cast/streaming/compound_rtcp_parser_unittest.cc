// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/compound_rtcp_parser.h"

#include <chrono>
#include <cmath>

#include "cast/streaming/mock_compound_rtcp_parser_client.h"
#include "cast/streaming/rtcp_session.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "util/chrono_helpers.h"

using testing::_;
using testing::Mock;
using testing::SaveArg;
using testing::StrictMock;

namespace openscreen {
namespace cast {
namespace {

constexpr Ssrc kSenderSsrc{1};
constexpr Ssrc kReceiverSsrc{2};

class CompoundRtcpParserTest : public testing::Test {
 public:
  RtcpSession* session() { return &session_; }
  StrictMock<MockCompoundRtcpParserClient>* client() { return &client_; }
  CompoundRtcpParser* parser() { return &parser_; }

 private:
  RtcpSession session_{kSenderSsrc, kReceiverSsrc, Clock::now()};
  StrictMock<MockCompoundRtcpParserClient> client_;
  CompoundRtcpParser parser_{&session_, &client_};
};

TEST_F(CompoundRtcpParserTest, ProcessesEmptyPacket) {
  const uint8_t kEmpty[0] = {};
  // Expect NO calls to mock client.
  EXPECT_TRUE(
      parser()->Parse(absl::Span<const uint8_t>(kEmpty, 0), FrameId::first()));
}

TEST_F(CompoundRtcpParserTest, ReturnsErrorForGarbage) {
  const uint8_t kGarbage[] = {
      0x42, 0x61, 0x16, 0x17, 0x26, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6d, 0x69,
      0x6e, 0x67, 0x2f, 0x63, 0x61, 0x73, 0x74, 0x2f, 0x63, 0x6f, 0x6d, 0x70,
      0x6f, 0x75, 0x6e, 0x64, 0x5f, 0x72, 0x74, 0x63, 0x70, 0x5f};
  // Expect NO calls to mock client.
  EXPECT_FALSE(parser()->Parse(kGarbage, FrameId::first()));
}

TEST_F(CompoundRtcpParserTest, ParsesReceiverReportWithoutReportBlock) {
  // clang-format off
  const uint8_t kReceiverReportWithoutReportBlock[] = {
      0b10000000,  // Version=2, Padding=no, ReportCount=0.
      201,  // RTCP Packet type byte.
      0x00, 0x01,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
  };
  // clang-format on

  // Expect NO calls to mock client.
  EXPECT_TRUE(
      parser()->Parse(kReceiverReportWithoutReportBlock, FrameId::first()));
}

TEST_F(CompoundRtcpParserTest, ParsesReceiverReportWithReportBlock) {
  // clang-format off
  const uint8_t kReceiverReportWithReportBlock[] = {
      0b10000001,  // Version=2, Padding=no, ReportCount=1.
      201,  // RTCP Packet type byte.
      0x00, 0x07,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.

      // Report block:
      0x00, 0x00, 0x00, 0x01,  // Sender SSRC.
      0x05,  // Fraction Lost.
      0x01, 0x02, 0x03,  // Cumulative # packets lost.
      0x09, 0x09, 0x09, 0x02,  // Highest sequence number.
      0x00, 0x00, 0x00, 0xaa,  // Interarrival Jitter.
      0x0b, 0x0c, 0x8f, 0xed,  // Sender Report ID.
      0x00, 0x01, 0x00, 0x00,  // Delay since last sender report.
  };
  // clang-format on

  RtcpReportBlock block;
  EXPECT_CALL(*(client()), OnReceiverReport(_)).WillOnce(SaveArg<0>(&block));
  EXPECT_TRUE(
      parser()->Parse(kReceiverReportWithReportBlock, FrameId::first()));
  Mock::VerifyAndClearExpectations(client());
  EXPECT_EQ(kSenderSsrc, block.ssrc);
  EXPECT_EQ(uint8_t{5}, block.packet_fraction_lost_numerator);
  EXPECT_EQ(0x010203, block.cumulative_packets_lost);
  EXPECT_EQ(uint32_t{0x09090902}, block.extended_high_sequence_number);
  EXPECT_EQ(RtpTimeDelta::FromTicks(170), block.jitter);
  EXPECT_EQ(StatusReportId{0x0b0c8fed}, block.last_status_report_id);
  EXPECT_EQ(RtcpReportBlock::Delay(65536), block.delay_since_last_report);
}

TEST_F(CompoundRtcpParserTest, ParsesPictureLossIndicatorMessage) {
  // clang-format off
  const uint8_t kPictureLossIndicatorPacket[] = {
      0b10000000 | 1,  // Version=2, Padding=no, Subtype=PLI.
      206,  // RTCP Packet type byte.
      0x00, 0x02,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x00, 0x00, 0x00, 0x01,  // Sender SSRC.
  };

  const uint8_t kPictureLossIndicatorPacketWithWrongReceiverSsrc[] = {
      0b10000000 | 1,  // Version=2, Padding=no, Subtype=PLI.
      206,  // RTCP Packet type byte.
      0x00, 0x02,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x03,  // WRONG Receiver SSRC.
      0x00, 0x00, 0x00, 0x01,  // Sender SSRC.
  };

  const uint8_t kPictureLossIndicatorPacketWithWrongSenderSsrc[] = {
      0b10000000 | 1,  // Version=2, Padding=no, Subtype=PLI.
      206,  // RTCP Packet type byte.
      0x00, 0x02,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x00, 0x00, 0x00, 0x03,  // WRONG Sender SSRC.
  };
  // clang-format on

  // The mock client should get a PLI notification when the packet is valid and
  // contains the correct SSRCs.
  EXPECT_CALL(*(client()), OnReceiverIndicatesPictureLoss());
  EXPECT_TRUE(parser()->Parse(kPictureLossIndicatorPacket, FrameId::first()));
  Mock::VerifyAndClearExpectations(client());

  // The mock client should get no PLI notifications when either of the SSRCs is
  // incorrect.
  EXPECT_CALL(*(client()), OnReceiverIndicatesPictureLoss()).Times(0);
  EXPECT_TRUE(parser()->Parse(kPictureLossIndicatorPacketWithWrongReceiverSsrc,
                              FrameId::first()));
  Mock::VerifyAndClearExpectations(client());
  EXPECT_CALL(*(client()), OnReceiverIndicatesPictureLoss()).Times(0);
  EXPECT_TRUE(parser()->Parse(kPictureLossIndicatorPacketWithWrongSenderSsrc,
                              FrameId::first()));
  Mock::VerifyAndClearExpectations(client());
}

// Tests that RTCP packets containing chronologically-old data are ignored. This
// test's methodology simulates a real-world possibility: A receiver sends a
// "Picture Loss Indicator" in one RTCP packet, and then it sends another packet
// ~1 second later without the PLI, indicating the problem has been resolved.
// However, the packets are delivered out-of-order by the network. In this case,
// the CompoundRtcpParser should ignore the stale packet containing the PLI.
TEST_F(CompoundRtcpParserTest, IgnoresStalePackets) {
  // clang-format off
  const uint8_t kNotStaleCompoundPacket[] = {
      // Receiver report:
      0b10000000,  // Version=2, Padding=no, ReportCount=0.
      201,  // RTCP Packet type byte.
      0x00, 0x01,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.

      // Receiver reference time report:
      0b10000000,  // Version=2, Padding=no.
      207,  // RTCP Packet type byte.
      0x00, 0x04,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x04,  // Block type = Receiver Reference Time Report
      0x00,  // Reserved byte.
      0x00, 0x02,  // Block length = 2.
      0xe0, 0x73, 0x2e, 0x54,  // NTP Timestamp (late evening on 2019-04-30).
          0x80, 0x00, 0x00, 0x00,
  };

  const uint8_t kStaleCompoundPacketWithPli[] = {
      // Picture loss indicator:
      0b10000000 | 1,  // Version=2, Padding=no, Subtype=PLI.
      206,  // RTCP Packet type byte.
      0x00, 0x02,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x00, 0x00, 0x00, 0x01,  // Sender SSRC.

      // Receiver reference time report:
      0b10000000,  // Version=2, Padding=no.
      207,  // RTCP Packet type byte.
      0x00, 0x04,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x04,  // Block type = Receiver Reference Time Report
      0x00,  // Reserved byte.
      0x00, 0x02,  // Block length = 2.
      0xe0, 0x73, 0x2e, 0x53,  // NTP Timestamp (late evening on 2019-04-30).
          0x42, 0x31, 0x20, 0x00,
  };
  // clang-format on

  const auto expected_timestamp =
      session()->ntp_converter().ToLocalTime(NtpTimestamp{0xe0732e5480000000});
  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(expected_timestamp));
  EXPECT_CALL(*(client()), OnReceiverIndicatesPictureLoss()).Times(0);
  EXPECT_TRUE(parser()->Parse(kNotStaleCompoundPacket, FrameId::first()));
  EXPECT_TRUE(parser()->Parse(kStaleCompoundPacketWithPli, FrameId::first()));
}

// Tests that unknown RTCP extended reports are ignored, but known ones are
// still parsed when sent alongside the unknown ones.
TEST_F(CompoundRtcpParserTest, IgnoresUnknownExtendedReports) {
  // clang-format off
  const uint8_t kPacketWithThreeExtendedReports[] = {
      0b10000000,  // Version=2, Padding=no.
      207,  // RTCP Packet type byte.
      0x00, 0x0c,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.

      // Unknown extended report:
      0x02,  // Block type = unknown (2)
      0x00,  // Reserved byte.
      0x00, 0x06,  // Block length = 6 words.
      0x01, 0x01, 0x01, 0x01,
      0x02, 0x02, 0x02, 0x02,
      0x03, 0x03, 0x03, 0x03,
      0x04, 0x04, 0x04, 0x04,
      0x05, 0x05, 0x05, 0x05,
      0x06, 0x06, 0x06, 0x06,

      // Receiver Reference Time Report:
      0x04,  // Block type = RRTR
      0x00,  // Reserved byte.
      0x00, 0x02,  // Block length = 2 words.
      0xe0, 0x73, 0x2e, 0x55,  // NTP Timestamp (late evening on 2019-04-30).
          0x00, 0x00, 0x00, 0x00,

      // Another unknown extended report:
      0x00,  // Block type = unknown (0)
      0x00,  // Reserved byte.
      0x00, 0x00,  // Block length = 0 words.
  };
  // clang-format on

  const auto expected_timestamp =
      session()->ntp_converter().ToLocalTime(NtpTimestamp{0xe0732e5500000000});
  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(expected_timestamp));
  EXPECT_TRUE(
      parser()->Parse(kPacketWithThreeExtendedReports, FrameId::first()));
}

// Tests that a simple Cast Feedback packet is parsed, and the checkpoint frame
// ID is properly bit-extended, based on the current state of the Sender.
TEST_F(CompoundRtcpParserTest, ParsesSimpleFeedback) {
  // clang-format off
  const uint8_t kFeedbackPacket[] = {
      0b10000000 | 15,  // Version=2, Padding=no, Subtype=Feedback.
      206,  // RTCP Packet type byte.
      0x00, 0x04,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x00, 0x00, 0x00, 0x01,  // Sender SSRC.
      'C', 'A', 'S', 'T',
      0x0a,  // Checkpoint Frame ID = 10.
      0x00,  // No NACKs.
      0x02, 0x26,  // Playout delay = 550 ms.
  };
  // clang-format on

  // First scenario: Valid range of FrameIds is [0,42].
  const auto kMaxFeedbackFrameId0 = FrameId::first() + 42;
  const auto expected_frame_id0 = FrameId::first() + 10;
  const auto expected_playout_delay = milliseconds(550);
  EXPECT_CALL(*(client()),
              OnReceiverCheckpoint(expected_frame_id0, expected_playout_delay));
  EXPECT_TRUE(parser()->Parse(kFeedbackPacket, kMaxFeedbackFrameId0));
  Mock::VerifyAndClearExpectations(client());

  // Second scenario: Valid range of FrameIds is [299,554]. Note: 544 == 0x22a.
  const auto kMaxFeedbackFrameId1 = FrameId::first() + 0x22a;
  const auto expected_frame_id1 = FrameId::first() + 0x20a;
  EXPECT_CALL(*(client()),
              OnReceiverCheckpoint(expected_frame_id1, expected_playout_delay));
  EXPECT_TRUE(parser()->Parse(kFeedbackPacket, kMaxFeedbackFrameId1));
  Mock::VerifyAndClearExpectations(client());
}

// Tests NACK feedback parsing, and that redundant NACKs are de-duped, and that
// the results are delivered to the client sorted.
TEST_F(CompoundRtcpParserTest, ParsesFeedbackWithNacks) {
  // clang-format off
  const uint8_t kFeedbackPacket[] = {
      0b10000000 | 15,  // Version=2, Padding=no, Subtype=Feedback.
      206,  // RTCP Packet type byte.
      0x00, 0x0b,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x00, 0x00, 0x00, 0x01,  // Sender SSRC.
      'C', 'A', 'S', 'T',
      0x0a,  // Checkpoint Frame ID = 10.
      0x07,  // Seven NACKs.
      0x02, 0x28,  // Playout delay = 552 ms.
      0x0b, 0x00, 0x03, 0b00000000,  // NACK Packet 3 in Frame 11.
      0x0b, 0x00, 0x07, 0b10001101,  // NACK Packet 7-8, 10-11, 15 in Frame 11.
      0x0d, 0xff, 0xff, 0b00000000,  // NACK all packets in Frame 13.
      0x0b, 0x00, 0x0b, 0b00000000,  // Redundant: NACK packet 11 in Frame 11.
      0x0c, 0xff, 0xff, 0b00000000,  // NACK all packets in Frame 12.
      0x0d, 0x00, 0x01, 0b00000000,  // Redundant: NACK packet 1 in Frame 13.
      0x0e, 0x00, 0x00, 0b01000010,  // NACK packets 0, 2, 7 in Frame 14.
  };
  // clang-format on

  // The de-duped and sorted list of the frame/packet NACKs expected when
  // parsing kFeedbackPacket:
  const std::vector<PacketNack> kMissingPackets = {
      {FrameId::first() + 11, 3},
      {FrameId::first() + 11, 7},
      {FrameId::first() + 11, 8},
      {FrameId::first() + 11, 10},
      {FrameId::first() + 11, 11},
      {FrameId::first() + 11, 15},
      {FrameId::first() + 12, kAllPacketsLost},
      {FrameId::first() + 13, kAllPacketsLost},
      {FrameId::first() + 14, 0},
      {FrameId::first() + 14, 2},
      {FrameId::first() + 14, 7},
  };

  const auto kMaxFeedbackFrameId = FrameId::first() + 42;
  const auto expected_frame_id = FrameId::first() + 10;
  const auto expected_playout_delay = milliseconds(552);
  EXPECT_CALL(*(client()),
              OnReceiverCheckpoint(expected_frame_id, expected_playout_delay));
  EXPECT_CALL(*(client()), OnReceiverIsMissingPackets(kMissingPackets));
  EXPECT_TRUE(parser()->Parse(kFeedbackPacket, kMaxFeedbackFrameId));
}

// Tests the CST2 "later frame ACK" parsing: Both the common "2 bytes of bit
// vector" case, and a "multiple words of bit vector" case.
TEST_F(CompoundRtcpParserTest, ParsesFeedbackWithAcks) {
  // clang-format off
  const uint8_t kSmallerFeedbackPacket[] = {
      0b10000000 | 15,  // Version=2, Padding=no, Subtype=Feedback.
      206,  // RTCP Packet type byte.
      0x00, 0x07,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x00, 0x00, 0x00, 0x01,  // Sender SSRC.
      'C', 'A', 'S', 'T',
      0x0a,  // Checkpoint Frame ID = 10.
      0x01,  // One NACK.
      0x01, 0x26,  // Playout delay = 294 ms.
      0x0b, 0x00, 0x03, 0b00000000,  // NACK Packet 3 in Frame 11.
      'C', 'S', 'T', '2',
      0x99,  // Feedback counter.
      0x02,  // 2 bytes of ACK bit vector.
      0b00000010, 0b00000000,  // ACK only frame 13.
  };

  const uint8_t kLargerFeedbackPacket[] = {
      0b10000000 | 15,  // Version=2, Padding=no, Subtype=Feedback.
      206,  // RTCP Packet type byte.
      0x00, 0x08,  // Length of remainder of packet, in 32-bit words.
      0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
      0x00, 0x00, 0x00, 0x01,  // Sender SSRC.
      'C', 'A', 'S', 'T',
      0x0a,  // Checkpoint Frame ID = 10.
      0x00,  // Zero NACKs.
      0x01, 0x26,  // Playout delay = 294 ms.
      'C', 'S', 'T', '2',
      0x99,  // Feedback counter.
      0x0a,  // 10 bytes of ACK bit vector.
      0b11111111, 0b11111111,  // ACK frames 12-27.
      0b00000000, 0b00000001, 0b00000000, 0b00000000,  // ACK frame 36.
      0b00000000, 0b00000000, 0b00000000, 0b10000000,  // ACK frame 91.
  };
  // clang-format on

  // From the smaller packet: The single frame ACK and single packet NACK.
  const std::vector<FrameId> kFrame13Only = {FrameId::first() + 13};
  const std::vector<PacketNack> kFrame11Packet3Only = {
      {FrameId::first() + 11, 3}};

  // From the larger packet: Many frame ACKs.
  const std::vector<FrameId> kManyFrames = {
      FrameId::first() + 12, FrameId::first() + 13, FrameId::first() + 14,
      FrameId::first() + 15, FrameId::first() + 16, FrameId::first() + 17,
      FrameId::first() + 18, FrameId::first() + 19, FrameId::first() + 20,
      FrameId::first() + 21, FrameId::first() + 22, FrameId::first() + 23,
      FrameId::first() + 24, FrameId::first() + 25, FrameId::first() + 26,
      FrameId::first() + 27, FrameId::first() + 36, FrameId::first() + 91,
  };

  // Test the smaller packet.
  const auto kMaxFeedbackFrameId = FrameId::first() + 100;
  const auto expected_frame_id = FrameId::first() + 10;
  const auto expected_playout_delay = milliseconds(294);
  EXPECT_CALL(*(client()),
              OnReceiverCheckpoint(expected_frame_id, expected_playout_delay));
  EXPECT_CALL(*(client()), OnReceiverHasFrames(kFrame13Only));
  EXPECT_CALL(*(client()), OnReceiverIsMissingPackets(kFrame11Packet3Only));
  EXPECT_TRUE(parser()->Parse(kSmallerFeedbackPacket, kMaxFeedbackFrameId));
  Mock::VerifyAndClearExpectations(client());

  // Test the larger ACK packet.
  EXPECT_CALL(*(client()),
              OnReceiverCheckpoint(expected_frame_id, expected_playout_delay));
  EXPECT_CALL(*(client()), OnReceiverHasFrames(kManyFrames));
  EXPECT_TRUE(parser()->Parse(kLargerFeedbackPacket, kMaxFeedbackFrameId));
  Mock::VerifyAndClearExpectations(client());
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
