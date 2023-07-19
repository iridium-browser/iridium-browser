// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/compound_rtcp_builder.h"

#include <algorithm>
#include <chrono>

#include "cast/streaming/compound_rtcp_parser.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/mock_compound_rtcp_parser_client.h"
#include "cast/streaming/rtcp_session.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "util/chrono_helpers.h"

using testing::_;
using testing::Invoke;
using testing::Mock;
using testing::SaveArg;
using testing::StrictMock;

namespace openscreen {
namespace cast {
namespace {

constexpr Ssrc kSenderSsrc{1};
constexpr Ssrc kReceiverSsrc{2};

class CompoundRtcpBuilderTest : public testing::Test {
 public:
  RtcpSession* session() { return &session_; }
  CompoundRtcpBuilder* builder() { return &builder_; }
  StrictMock<MockCompoundRtcpParserClient>* client() { return &client_; }
  CompoundRtcpParser* parser() { return &parser_; }

  // Return |timestamp| converted to the NtpTimestamp wire format and then
  // converted back to the local Clock's time_point. The result will be either
  // exactly equal to |original|, or one tick off from it due to the lossy
  // conversions.
  Clock::time_point ViaNtpTimestampTranslation(
      Clock::time_point timestamp) const {
    return session_.ntp_converter().ToLocalTime(
        session_.ntp_converter().ToNtpTimestamp(timestamp));
  }

 private:
  RtcpSession session_{kSenderSsrc, kReceiverSsrc, Clock::now()};
  CompoundRtcpBuilder builder_{&session_};
  StrictMock<MockCompoundRtcpParserClient> client_;
  CompoundRtcpParser parser_{&session_, &client_};
};

// Tests that the builder, by default, produces RTCP packets that always include
// the receiver's reference time and checkpoint information.
TEST_F(CompoundRtcpBuilderTest, TheBasics) {
  const FrameId checkpoint = FrameId::first() + 42;
  builder()->SetCheckpointFrame(checkpoint);
  const milliseconds playout_delay{321};
  builder()->SetPlayoutDelay(playout_delay);

  const auto send_time = Clock::now();
  uint8_t buffer[CompoundRtcpBuilder::kRequiredBufferSize];
  const auto packet = builder()->BuildPacket(send_time, buffer);
  ASSERT_TRUE(packet.data());

  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(
                               ViaNtpTimestampTranslation(send_time)));
  EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, playout_delay));
  ASSERT_TRUE(parser()->Parse(packet, checkpoint));
}

// Tests that the builder correctly serializes a Receiver Report Block and
// includes it only in the next-built RTCP packet.
TEST_F(CompoundRtcpBuilderTest, WithReceiverReportBlock) {
  const FrameId checkpoint = FrameId::first() + 42;
  builder()->SetCheckpointFrame(checkpoint);
  const auto playout_delay = builder()->playout_delay();

  RtcpReportBlock original;
  original.ssrc = kSenderSsrc;
  original.packet_fraction_lost_numerator = 1;
  original.cumulative_packets_lost = 2;
  original.extended_high_sequence_number = 3;
  original.jitter = RtpTimeDelta::FromTicks(4);
  original.last_status_report_id = StatusReportId{0x05060708};
  original.delay_since_last_report = RtcpReportBlock::Delay(9);
  builder()->IncludeReceiverReportInNextPacket(original);

  const auto send_time = Clock::now();
  uint8_t buffer[CompoundRtcpBuilder::kRequiredBufferSize];
  const auto packet = builder()->BuildPacket(send_time, buffer);
  ASSERT_TRUE(packet.data());

  // Expect that the builder has produced a RTCP packet that includes the
  // receiver report block.
  const auto max_feedback_frame_id = checkpoint + 2;
  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(
                               ViaNtpTimestampTranslation(send_time)));
  EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, playout_delay));
  RtcpReportBlock parsed;
  EXPECT_CALL(*(client()), OnReceiverReport(_)).WillOnce(SaveArg<0>(&parsed));
  ASSERT_TRUE(parser()->Parse(packet, max_feedback_frame_id));
  Mock::VerifyAndClearExpectations(client());
  EXPECT_EQ(original.ssrc, parsed.ssrc);
  EXPECT_EQ(original.packet_fraction_lost_numerator,
            parsed.packet_fraction_lost_numerator);
  EXPECT_EQ(original.cumulative_packets_lost, parsed.cumulative_packets_lost);
  EXPECT_EQ(original.extended_high_sequence_number,
            parsed.extended_high_sequence_number);
  EXPECT_EQ(original.jitter, parsed.jitter);
  EXPECT_EQ(original.last_status_report_id, parsed.last_status_report_id);
  EXPECT_EQ(original.delay_since_last_report, parsed.delay_since_last_report);

  // Build again, but this time the builder should not include the receiver
  // report block.
  const auto second_send_time = send_time + milliseconds(500);
  const auto second_packet = builder()->BuildPacket(second_send_time, buffer);
  ASSERT_TRUE(second_packet.data());
  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(
                               ViaNtpTimestampTranslation(second_send_time)));
  EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, playout_delay));
  EXPECT_CALL(*(client()), OnReceiverReport(_)).Times(0);
  ASSERT_TRUE(parser()->Parse(second_packet, max_feedback_frame_id));
  Mock::VerifyAndClearExpectations(client());
}

// Tests that the builder repeatedly produces packets with the PLI message as
// long as the PLI flag is set, and produces packets without the PLI message
// while the flag is not set.
TEST_F(CompoundRtcpBuilderTest, WithPictureLossIndicator) {
  // Turn the PLI flag off and on twice, generating several packets while the
  // flag is in each state.
  FrameId checkpoint = FrameId::first();
  auto send_time = Clock::now();
  uint8_t buffer[CompoundRtcpBuilder::kRequiredBufferSize];
  for (int status = 0; status <= 3; ++status) {
    const bool pli_flag_set = ((status % 2) != 0);
    builder()->SetPictureLossIndicator(pli_flag_set);

    // Produce three packets while the PLI flag is not changing, and confirm the
    // PLI condition is being parsed on the other end.
    for (int i = 0; i < 3; ++i) {
      SCOPED_TRACE(testing::Message() << "status=" << status << ", i=" << i);

      EXPECT_EQ(pli_flag_set, builder()->is_picture_loss_indicator_set());
      builder()->SetCheckpointFrame(checkpoint);
      const auto playout_delay = builder()->playout_delay();
      const auto packet = builder()->BuildPacket(send_time, buffer);
      ASSERT_TRUE(packet.data());

      const auto max_feedback_frame_id = checkpoint + 1;
      EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(
                                   ViaNtpTimestampTranslation(send_time)));
      EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, playout_delay));
      EXPECT_CALL(*(client()), OnReceiverIndicatesPictureLoss())
          .Times(pli_flag_set ? 1 : 0);
      ASSERT_TRUE(parser()->Parse(packet, max_feedback_frame_id));
      Mock::VerifyAndClearExpectations(client());

      ++checkpoint;
      send_time += milliseconds(500);
    }
  }
}

// Tests that the builder produces packets with frame-level and specific-packet
// NACKs, but includes this information only in the next-built RTCP packet.
TEST_F(CompoundRtcpBuilderTest, WithNacks) {
  const FrameId checkpoint = FrameId::first() + 15;
  builder()->SetCheckpointFrame(checkpoint);
  const auto playout_delay = builder()->playout_delay();

  const std::vector<PacketNack> kPacketNacks = {
      {FrameId::first() + 16, FramePacketId{0}},
      {FrameId::first() + 16, FramePacketId{1}},
      {FrameId::first() + 16, FramePacketId{2}},
      {FrameId::first() + 16, FramePacketId{7}},
      {FrameId::first() + 16, FramePacketId{15}},
      {FrameId::first() + 17, FramePacketId{19}},
      {FrameId::first() + 18, kAllPacketsLost},
      {FrameId::first() + 19, kAllPacketsLost},
  };
  builder()->IncludeFeedbackInNextPacket(kPacketNacks, {});

  const auto send_time = Clock::now();
  uint8_t buffer[CompoundRtcpBuilder::kRequiredBufferSize];
  const auto packet = builder()->BuildPacket(send_time, buffer);
  ASSERT_TRUE(packet.data());

  // Expect that the builder has produced a RTCP packet that also includes the
  // NACK feedback.
  const auto kMaxFeedbackFrameId = FrameId::first() + 19;
  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(
                               ViaNtpTimestampTranslation(send_time)));
  EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, playout_delay));
  EXPECT_CALL(*(client()), OnReceiverIsMissingPackets(kPacketNacks));
  ASSERT_TRUE(parser()->Parse(packet, kMaxFeedbackFrameId));
  Mock::VerifyAndClearExpectations(client());

  // Build again, but this time the builder should not include the feedback.
  const auto second_send_time = send_time + milliseconds(500);
  const auto second_packet = builder()->BuildPacket(second_send_time, buffer);
  ASSERT_TRUE(second_packet.data());
  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(
                               ViaNtpTimestampTranslation(second_send_time)));
  EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, playout_delay));
  EXPECT_CALL(*(client()), OnReceiverIsMissingPackets(_)).Times(0);
  ASSERT_TRUE(parser()->Parse(second_packet, kMaxFeedbackFrameId));
  Mock::VerifyAndClearExpectations(client());
}

// Tests that the builder produces packets with frame-level ACKs, but includes
// this information only in the next-built RTCP packet. Both a single-frame ACK
// and a multi-frame ACK are tested, to exercise the various code paths
// containing the serialization logic that auto-extends the ACK bit vector
// length when necessary.
TEST_F(CompoundRtcpBuilderTest, WithAcks) {
  const FrameId checkpoint = FrameId::first() + 22;
  builder()->SetCheckpointFrame(checkpoint);
  const auto playout_delay = builder()->playout_delay();

  const std::vector<FrameId> kTestCases[] = {
      // One frame ACK will result in building an ACK bit vector of 2 bytes
      // only.
      {FrameId::first() + 24},

      // These frame ACKs were chosen so that the ACK bit vector must expand to
      // be 6 (2 + 4) bytes long.
      {FrameId::first() + 25, FrameId::first() + 42, FrameId::first() + 43},
  };
  const auto kMaxFeedbackFrameId = FrameId::first() + 50;
  auto send_time = Clock::now();
  uint8_t buffer[CompoundRtcpBuilder::kRequiredBufferSize];
  for (const std::vector<FrameId>& frame_acks : kTestCases) {
    // Include the frame ACK feedback, and expect that the builder will produce
    // a RTCP packet that also includes the ACK feedback.
    builder()->IncludeFeedbackInNextPacket({}, frame_acks);
    const auto packet = builder()->BuildPacket(send_time, buffer);
    ASSERT_TRUE(packet.data());
    EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(
                                 ViaNtpTimestampTranslation(send_time)));
    EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, playout_delay));
    EXPECT_CALL(*(client()), OnReceiverHasFrames(frame_acks));
    ASSERT_TRUE(parser()->Parse(packet, kMaxFeedbackFrameId));
    Mock::VerifyAndClearExpectations(client());

    // Build again, but this time the builder should not include the feedback
    // because it was already provided in the prior packet.
    send_time += milliseconds(500);
    const auto second_packet = builder()->BuildPacket(send_time, buffer);
    ASSERT_TRUE(second_packet.data());
    EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(
                                 ViaNtpTimestampTranslation(send_time)));
    EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, playout_delay));
    EXPECT_CALL(*(client()), OnReceiverHasFrames(_)).Times(0);
    ASSERT_TRUE(parser()->Parse(second_packet, kMaxFeedbackFrameId));
    Mock::VerifyAndClearExpectations(client());

    send_time += milliseconds(500);
  }
}

// Tests that the builder handles scenarios where the provided buffer isn't big
// enough to hold all the ACK/NACK details. The expected behavior is that it
// will include as many of the NACKs as possible, followed by as many of the
// ACKs as possible.
TEST_F(CompoundRtcpBuilderTest, WithEverythingThatCanFit) {
  const FrameId checkpoint = FrameId::first();
  builder()->SetCheckpointFrame(checkpoint);

  // For this test, use an abnormally-huge, but not impossible, list of NACKs
  // and ACKs. Each NACK is for a separate frame so that a separate "loss field"
  // will be generated in the serialized output.
  std::vector<PacketNack> nacks;
  for (FrameId f = checkpoint + 1; f != checkpoint + 64; ++f) {
    nacks.push_back(PacketNack{f, FramePacketId{0}});
  }
  std::vector<FrameId> acks;
  for (FrameId f = checkpoint + 64; f < checkpoint + kMaxUnackedFrames; ++f) {
    acks.push_back(f);
  }
  ASSERT_FALSE(acks.empty());

  const auto max_feedback_frame_id = checkpoint + kMaxUnackedFrames;

  // First test: Include too many NACKs so that some of them will be dropped and
  // none of the ACKs will be included.
  builder()->IncludeFeedbackInNextPacket(nacks, acks);
  uint8_t buffer[CompoundRtcpBuilder::kRequiredBufferSize];
  const auto packet = builder()->BuildPacket(Clock::now(), buffer);
  ASSERT_TRUE(packet.data());
  EXPECT_EQ(sizeof(buffer), packet.size());  // The whole buffer should be used.

  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(_));
  EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, _));
  // No ACKs could be included.
  EXPECT_CALL(*(client()), OnReceiverHasFrames(_)).Times(0);
  EXPECT_CALL(*(client()), OnReceiverIsMissingPackets(_))
      .WillOnce(Invoke([&](std::vector<PacketNack> parsed_nacks) {
        // Some should be dropped.
        ASSERT_LT(parsed_nacks.size(), nacks.size());
        EXPECT_TRUE(std::equal(parsed_nacks.begin(), parsed_nacks.end(),
                               nacks.begin()));
      }));
  ASSERT_TRUE(parser()->Parse(packet, max_feedback_frame_id));
  Mock::VerifyAndClearExpectations(client());

  // Second test: Include fewer NACKs this time, so that none of the NACKs are
  // dropped, but not all of the ACKs can be included. With internal knowledge
  // of the wire format, it turns out that limiting serialization to 48 loss
  // fields will free-up just enough space for 2 bytes of ACK bit vector.
  constexpr int kFewerNackCount = 48;
  builder()->IncludeFeedbackInNextPacket(
      std::vector<PacketNack>(nacks.begin(), nacks.begin() + kFewerNackCount),
      acks);
  const auto second_packet = builder()->BuildPacket(Clock::now(), buffer);
  ASSERT_TRUE(second_packet.data());
  // The whole buffer should be used.
  EXPECT_EQ(sizeof(buffer), second_packet.size());

  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(_));
  EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, _));
  EXPECT_CALL(*(client()), OnReceiverHasFrames(_))
      .WillOnce(Invoke([&](std::vector<FrameId> parsed_acks) {
        // Some of the ACKs should be dropped.
        ASSERT_LT(parsed_acks.size(), acks.size());
        EXPECT_TRUE(
            std::equal(parsed_acks.begin(), parsed_acks.end(), acks.begin()));
      }));
  EXPECT_CALL(*(client()), OnReceiverIsMissingPackets(_))
      .WillOnce(Invoke([&](absl::Span<const PacketNack> parsed_nacks) {
        // All of the 48 NACKs provided should be present.
        ASSERT_EQ(kFewerNackCount, static_cast<int>(parsed_nacks.size()));
        EXPECT_TRUE(std::equal(parsed_nacks.begin(), parsed_nacks.end(),
                               nacks.begin()));
      }));
  ASSERT_TRUE(parser()->Parse(second_packet, max_feedback_frame_id));
  Mock::VerifyAndClearExpectations(client());

  // Third test: Include even fewer NACKs, so that nothing is dropped.
  constexpr int kEvenFewerNackCount = 46;
  builder()->IncludeFeedbackInNextPacket(
      std::vector<PacketNack>(nacks.begin(),
                              nacks.begin() + kEvenFewerNackCount),
      acks);
  const auto third_packet = builder()->BuildPacket(Clock::now(), buffer);
  ASSERT_TRUE(third_packet.data());

  EXPECT_CALL(*(client()), OnReceiverReferenceTimeAdvanced(_));
  EXPECT_CALL(*(client()), OnReceiverCheckpoint(checkpoint, _));
  EXPECT_CALL(*(client()), OnReceiverHasFrames(_))
      .WillOnce(Invoke([&](std::vector<FrameId> parsed_acks) {
        // All acks should be present.
        EXPECT_EQ(acks, parsed_acks);
      }));
  EXPECT_CALL(*(client()), OnReceiverIsMissingPackets(_))
      .WillOnce(Invoke([&](absl::Span<const PacketNack> parsed_nacks) {
        // Only the first 46 NACKs provided should be present.
        ASSERT_EQ(kEvenFewerNackCount, static_cast<int>(parsed_nacks.size()));
        EXPECT_TRUE(std::equal(parsed_nacks.begin(), parsed_nacks.end(),
                               nacks.begin()));
      }));
  ASSERT_TRUE(parser()->Parse(third_packet, max_feedback_frame_id));
  Mock::VerifyAndClearExpectations(client());
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
