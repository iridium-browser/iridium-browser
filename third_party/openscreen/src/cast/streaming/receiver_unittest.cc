// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver.h"

#include <stdint.h>

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

#include "absl/types/span.h"
#include "cast/streaming/compound_rtcp_parser.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/encoded_frame.h"
#include "cast/streaming/frame_crypto.h"
#include "cast/streaming/mock_environment.h"
#include "cast/streaming/receiver_packet_router.h"
#include "cast/streaming/rtcp_common.h"
#include "cast/streaming/rtcp_session.h"
#include "cast/streaming/rtp_defines.h"
#include "cast/streaming/rtp_packetizer.h"
#include "cast/streaming/rtp_time.h"
#include "cast/streaming/sender_report_builder.h"
#include "cast/streaming/session_config.h"
#include "cast/streaming/ssrc.h"
#include "cast/streaming/testing/simple_socket_subscriber.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "platform/api/udp_socket.h"
#include "platform/base/error.h"
#include "platform/base/ip_address.h"
#include "platform/base/span.h"
#include "platform/base/udp_packet.h"
#include "platform/test/byte_view_test_util.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"

using testing::_;
using testing::AtLeast;
using testing::Gt;
using testing::Invoke;
using testing::SaveArg;

namespace openscreen {
namespace cast {
namespace {

// Receiver configuration.

constexpr Ssrc kSenderSsrc = 1;
constexpr Ssrc kReceiverSsrc = 2;
constexpr int kRtpTimebase = 48000;
constexpr milliseconds kTargetPlayoutDelay{100};
constexpr auto kAesKey =
    std::array<uint8_t, 16>{{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f}};
constexpr auto kCastIvMask =
    std::array<uint8_t, 16>{{0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80,
                             0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00}};

constexpr milliseconds kTargetPlayoutDelayChange{800};
// Additional configuration for the Sender.
constexpr RtpPayloadType kRtpPayloadType = RtpPayloadType::kVideoVp8;
constexpr int kMaxRtpPacketSize = 64;

// A simulated one-way network delay, and round-trip network delay.
constexpr auto kOneWayNetworkDelay = milliseconds(3);
constexpr auto kRoundTripNetworkDelay = 2 * kOneWayNetworkDelay;
static_assert(kRoundTripNetworkDelay < kTargetPlayoutDelay &&
                  kRoundTripNetworkDelay < kTargetPlayoutDelayChange,
              "Network delay must be smaller than target playout delay.");

// An EncodedFrame for unit testing, one of a sequence of simulated frames, each
// of 10 ms duration. The first frame will be a key frame; and any later frames
// will be non-key, dependent on the prior frame. Frame 5 (the 6th frame in the
// zero-based sequence) will include a target playout delay change, an increase
// to 800 ms. Frames with different IDs will contain vary in their payload data
// size, but are always 3 or more packets' worth of data.
struct SimulatedFrame : public EncodedFrame {
  static constexpr milliseconds kFrameDuration = milliseconds(10);
  static constexpr milliseconds kTargetPlayoutDelayChange = milliseconds(800);

  static constexpr int kPlayoutChangeAtFrame = 5;

  SimulatedFrame(Clock::time_point first_frame_reference_time, int which) {
    frame_id = FrameId::first() + which;
    if (which == 0) {
      dependency = EncodedFrame::Dependency::kKeyFrame;
      referenced_frame_id = frame_id;
    } else {
      dependency = EncodedFrame::Dependency::kDependent;
      referenced_frame_id = frame_id - 1;
    }
    rtp_timestamp =
        GetRtpStartTime() +
        RtpTimeDelta::FromDuration(kFrameDuration * which, kRtpTimebase);
    reference_time = first_frame_reference_time + kFrameDuration * which;
    if (which == kPlayoutChangeAtFrame) {
      new_playout_delay = kTargetPlayoutDelayChange;
    }
    constexpr int kAdditionalBytesEachSuccessiveFrame = 3;
    buffer_.resize(3 * kMaxRtpPacketSize +
                   which * kAdditionalBytesEachSuccessiveFrame);
    for (size_t i = 0; i < buffer_.size(); ++i) {
      buffer_[i] = static_cast<uint8_t>(which + static_cast<int>(i));
    }
    data = buffer_;
  }

  static RtpTimeTicks GetRtpStartTime() {
    return RtpTimeTicks::FromTimeSinceOrigin(seconds(0), kRtpTimebase);
  }

  static milliseconds GetExpectedPlayoutDelay(int which) {
    return (which < kPlayoutChangeAtFrame) ? kTargetPlayoutDelay
                                           : kTargetPlayoutDelayChange;
  }

 private:
  std::vector<uint8_t> buffer_;
};

// static
constexpr milliseconds SimulatedFrame::kFrameDuration;
constexpr milliseconds SimulatedFrame::kTargetPlayoutDelayChange;
constexpr int SimulatedFrame::kPlayoutChangeAtFrame;

// Processes packets from the Receiver under test, as a real Sender might, and
// allows the unit tests to set expectations on events of interest to confirm
// proper behavior of the Receiver.
class MockSender : public CompoundRtcpParser::Client {
 public:
  MockSender(TaskRunner* task_runner, UdpSocket::Client* receiver)
      : task_runner_(task_runner),
        receiver_(receiver),
        sender_endpoint_{
            // Use a random IPv6 address in the range reserved for
            // "documentation purposes." Thus, the following is a fake address
            // that should be blocked by the OS (and all network packet
            // routers). But, these tests don't use real sockets, so...
            IPAddress::Parse("2001:db8:0d93:69c2:fd1a:49a6:a7c0:e8a6").value(),
            2344},
        rtcp_session_(kSenderSsrc, kReceiverSsrc, FakeClock::now()),
        sender_report_builder_(&rtcp_session_),
        rtcp_parser_(&rtcp_session_, this),
        crypto_(kAesKey, kCastIvMask),
        rtp_packetizer_(kRtpPayloadType, kSenderSsrc, kMaxRtpPacketSize) {}

  ~MockSender() override = default;

  void set_max_feedback_frame_id(FrameId f) { max_feedback_frame_id_ = f; }

  // Called by the test procedures to generate a Sender Report containing the
  // given lip-sync timestamps, and send it to the Receiver. The caller must
  // spin the TaskRunner for the RTCP packet to be delivered to the Receiver.
  StatusReportId SendSenderReport(Clock::time_point reference_time,
                                  RtpTimeTicks rtp_timestamp) {
    // Generate the Sender Report RTCP packet.
    uint8_t buffer[kMaxRtpPacketSizeForIpv4UdpOnEthernet];
    RtcpSenderReport sender_report;
    sender_report.reference_time = reference_time;
    sender_report.rtp_timestamp = rtp_timestamp;
    const auto packet_and_report_id =
        sender_report_builder_.BuildPacket(sender_report, buffer);

    // Send the RTCP packet as a UdpPacket directly to the Receiver instance.
    UdpPacket packet_to_send(packet_and_report_id.first.begin(),
                             packet_and_report_id.first.end());
    packet_to_send.set_source(sender_endpoint_);
    task_runner_->PostTaskWithDelay(
        [receiver = receiver_, packet = std::move(packet_to_send)]() mutable {
          receiver->OnRead(nullptr, ErrorOr<UdpPacket>(std::move(packet)));
        },
        kOneWayNetworkDelay);

    return packet_and_report_id.second;
  }

  // Sets which frame is currently being sent by this MockSender. Test code must
  // call SendRtpPackets() to send the packets.
  void SetFrameBeingSent(const EncodedFrame& frame) {
    frame_being_sent_ = crypto_.Encrypt(frame);
  }

  // Returns a vector containing each packet ID once (of the current frame being
  // sent). |permutation| controls the sort order of the vector: zero will
  // provide all the packet IDs in order, and greater values will provide them
  // in a different, predictable order.
  std::vector<FramePacketId> GetAllPacketIds(int permutation) {
    const int num_packets =
        rtp_packetizer_.ComputeNumberOfPackets(frame_being_sent_);
    OSP_CHECK_GT(num_packets, 0);
    std::vector<FramePacketId> ids;
    ids.reserve(num_packets);
    const FramePacketId last_packet_id =
        static_cast<FramePacketId>(num_packets - 1);
    for (FramePacketId packet_id = 0; packet_id <= last_packet_id;
         ++packet_id) {
      ids.push_back(packet_id);
    }
    for (int i = 0; i < permutation; ++i) {
      std::next_permutation(ids.begin(), ids.end());
    }
    return ids;
  }

  // Send the specified packets of the current frame being sent.
  void SendRtpPackets(const std::vector<FramePacketId>& packets_to_send) {
    uint8_t buffer[kMaxRtpPacketSize];
    for (FramePacketId packet_id : packets_to_send) {
      const auto span =
          rtp_packetizer_.GeneratePacket(frame_being_sent_, packet_id, buffer);
      UdpPacket packet_to_send(span.begin(), span.end());
      packet_to_send.set_source(sender_endpoint_);
      task_runner_->PostTaskWithDelay(
          [receiver = receiver_, packet = std::move(packet_to_send)]() mutable {
            receiver->OnRead(nullptr, ErrorOr<UdpPacket>(std::move(packet)));
          },
          kOneWayNetworkDelay);
    }
  }

  // Called to process a packet from the Receiver.
  void OnPacketFromReceiver(absl::Span<const uint8_t> packet) {
    EXPECT_TRUE(rtcp_parser_.Parse(packet, max_feedback_frame_id_));
  }

  // CompoundRtcpParser::Client implementation: Tests set expectations on these
  // mocks to confirm that the receiver is providing the right data to the
  // sender in its RTCP packets.
  MOCK_METHOD1(OnReceiverReferenceTimeAdvanced,
               void(Clock::time_point reference_time));
  MOCK_METHOD1(OnReceiverReport, void(const RtcpReportBlock& receiver_report));
  MOCK_METHOD0(OnReceiverIndicatesPictureLoss, void());
  MOCK_METHOD2(OnReceiverCheckpoint,
               void(FrameId frame_id, milliseconds playout_delay));
  MOCK_METHOD1(OnReceiverHasFrames, void(std::vector<FrameId> acks));
  MOCK_METHOD1(OnReceiverIsMissingPackets, void(std::vector<PacketNack> nacks));

 private:
  TaskRunner* const task_runner_;
  UdpSocket::Client* const receiver_;
  const IPEndpoint sender_endpoint_;
  RtcpSession rtcp_session_;
  SenderReportBuilder sender_report_builder_;
  CompoundRtcpParser rtcp_parser_;
  FrameCrypto crypto_;
  RtpPacketizer rtp_packetizer_;
  FrameId max_feedback_frame_id_ = FrameId::first() + kMaxUnackedFrames;

  EncryptedFrame frame_being_sent_;
};

class MockConsumer : public Receiver::Consumer {
 public:
  MOCK_METHOD1(OnFramesReady, void(int next_frame_buffer_size));
};

class ReceiverTest : public testing::Test {
 public:
  ReceiverTest()
      : clock_(Clock::now()),
        task_runner_(&clock_),
        env_(&FakeClock::now, &task_runner_),
        packet_router_(&env_),
        receiver_(&env_,
                  &packet_router_,
                  {/* .sender_ssrc = */ kSenderSsrc,
                   /* .receiver_ssrc = */ kReceiverSsrc,
                   /* .rtp_timebase = */ kRtpTimebase,
                   /* .channels = */ 2,
                   /* .target_playout_delay = */ kTargetPlayoutDelay,
                   /* .aes_secret_key = */ kAesKey,
                   /* .aes_iv_mask = */ kCastIvMask,
                   /* .is_pli_enabled = */ true}),
        sender_(&task_runner_, &env_) {
    env_.SetSocketSubscriber(&socket_subscriber_);
    ON_CALL(env_, SendPacket(_))
        .WillByDefault(Invoke([this](ByteView packet) {
          task_runner_.PostTaskWithDelay(
              [sender = &sender_, copy_of_packet = std::vector<uint8_t>(
                                      packet.begin(), packet.end())]() mutable {
                sender->OnPacketFromReceiver(std::move(copy_of_packet));
              },
              kOneWayNetworkDelay);
        }));
    receiver_.SetConsumer(&consumer_);
  }

  ~ReceiverTest() override = default;

  Receiver* receiver() { return &receiver_; }
  MockSender* sender() { return &sender_; }
  MockConsumer* consumer() { return &consumer_; }

  void AdvanceClockAndRunTasks(Clock::duration delta) { clock_.Advance(delta); }
  void RunTasksUntilIdle() { task_runner_.RunTasksUntilIdle(); }

  // Sends the initial Sender Report with lip-sync timing information to
  // "unblock" the Receiver, and confirms the Receiver immediately replies with
  // a corresponding Receiver Report.
  void ExchangeInitialReportPackets() {
    const Clock::time_point start_time = FakeClock::now();
    sender_.SendSenderReport(start_time, SimulatedFrame::GetRtpStartTime());
    AdvanceClockAndRunTasks(
        kOneWayNetworkDelay);  // Transmit report to Receiver.
    // The Receiver will immediately reply with a Receiver Report.
    EXPECT_CALL(sender_,
                OnReceiverCheckpoint(FrameId::leader(), kTargetPlayoutDelay))
        .Times(1);
    AdvanceClockAndRunTasks(kOneWayNetworkDelay);  // Transmit reply to Sender.
    testing::Mock::VerifyAndClearExpectations(&sender_);
  }

  // Consume one frame from the Receiver, and verify that it is the same as the
  // |sent_frame|. Exception: The |reference_time| is the playout time on the
  // Receiver's end, while it refers to the capture time on the Sender's end.
  void ConsumeAndVerifyFrame(const SimulatedFrame& sent_frame) {
    SCOPED_TRACE(testing::Message() << "for frame " << sent_frame.frame_id);

    const int payload_size = receiver()->AdvanceToNextFrame();
    ASSERT_NE(Receiver::kNoFramesReady, payload_size);
    std::vector<uint8_t> buffer(payload_size);
    EncodedFrame received_frame = receiver()->ConsumeNextFrame(buffer);

    EXPECT_EQ(sent_frame.dependency, received_frame.dependency);
    EXPECT_EQ(sent_frame.frame_id, received_frame.frame_id);
    EXPECT_EQ(sent_frame.referenced_frame_id,
              received_frame.referenced_frame_id);
    EXPECT_EQ(sent_frame.rtp_timestamp, received_frame.rtp_timestamp);
    EXPECT_EQ(sent_frame.reference_time + kOneWayNetworkDelay +
                  SimulatedFrame::GetExpectedPlayoutDelay(sent_frame.frame_id -
                                                          FrameId::first()),
              received_frame.reference_time);
    EXPECT_EQ(sent_frame.new_playout_delay, received_frame.new_playout_delay);
    ExpectByteViewsHaveSameBytes(sent_frame.data, received_frame.data);
  }

  // Consume zero or more frames from the Receiver, verifying that they are the
  // same as the SimulatedFrame that was sent.
  void ConsumeAndVerifyFrames(int first,
                              int last,
                              Clock::time_point start_time) {
    for (int i = first; i <= last; ++i) {
      ConsumeAndVerifyFrame(SimulatedFrame(start_time, i));
    }
  }

 private:
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  testing::NiceMock<MockEnvironment> env_;
  ReceiverPacketRouter packet_router_;
  Receiver receiver_;
  testing::NiceMock<MockSender> sender_;
  testing::NiceMock<MockConsumer> consumer_;
  SimpleSubscriber socket_subscriber_;
};

// Tests that the Receiver processes RTCP packets correctly and sends RTCP
// reports at regular intervals.
TEST_F(ReceiverTest, ReceivesAndSendsRtcpPackets) {
  // Sender-side expectations, after the Receiver has processed the first Sender
  // Report.
  Clock::time_point receiver_reference_time{};
  EXPECT_CALL(*sender(), OnReceiverReferenceTimeAdvanced(_))
      .WillOnce(SaveArg<0>(&receiver_reference_time));
  RtcpReportBlock receiver_report;
  EXPECT_CALL(*sender(), OnReceiverReport(_))
      .WillOnce(SaveArg<0>(&receiver_report));
  EXPECT_CALL(*sender(),
              OnReceiverCheckpoint(FrameId::leader(), kTargetPlayoutDelay))
      .Times(1);

  // Have the MockSender send a Sender Report with lip-sync timing information.
  const Clock::time_point sender_reference_time = FakeClock::now();
  const RtpTimeTicks sender_rtp_timestamp =
      RtpTimeTicks::FromTimeSinceOrigin(seconds(1), kRtpTimebase);
  const StatusReportId sender_report_id =
      sender()->SendSenderReport(sender_reference_time, sender_rtp_timestamp);

  AdvanceClockAndRunTasks(kRoundTripNetworkDelay);

  // Expect the MockSender got back a Receiver Report that includes its SSRC and
  // the last Sender Report ID.
  testing::Mock::VerifyAndClearExpectations(sender());
  EXPECT_EQ(kSenderSsrc, receiver_report.ssrc);
  EXPECT_EQ(sender_report_id, receiver_report.last_status_report_id);

  // Confirm the clock offset math: Since the Receiver and MockSender share the
  // same underlying FakeClock, the Receiver should be ahead of the Sender,
  // which reflects the simulated one-way network packet travel time (of the
  // Sender Report).
  //
  // Note: The offset can be affected by the lossy conversion when going to and
  // from the wire-format NtpTimestamps. See the unit tests in
  // ntp_time_unittest.cc for further discussion.
  constexpr auto kAllowedNtpRoundingError = microseconds(2);
  EXPECT_NEAR(to_microseconds(kOneWayNetworkDelay).count(),
              static_cast<double>(to_microseconds(receiver_reference_time -
                                                  sender_reference_time)
                                      .count()),
              kAllowedNtpRoundingError.count());

  // Without the Sender doing anything, the Receiver should continue providing
  // RTCP reports at regular intervals. Simulate three intervals of time,
  // verifying that the Receiver did send reports.
  Clock::time_point last_receiver_reference_time = receiver_reference_time;
  for (int i = 0; i < 3; ++i) {
    receiver_reference_time = Clock::time_point();
    EXPECT_CALL(*sender(), OnReceiverReferenceTimeAdvanced(_))
        .WillRepeatedly(SaveArg<0>(&receiver_reference_time));
    AdvanceClockAndRunTasks(kRtcpReportInterval);
    testing::Mock::VerifyAndClearExpectations(sender());
    EXPECT_LT(last_receiver_reference_time, receiver_reference_time);
    last_receiver_reference_time = receiver_reference_time;
  }
}

// Tests that the Receiver processes RTP packets, which might arrive in-order or
// out of order, but such that each frame is completely received in-order. Also,
// confirms that target playout delay changes are processed/applied correctly.
TEST_F(ReceiverTest, ReceivesFramesInOrder) {
  const Clock::time_point start_time = FakeClock::now();
  ExchangeInitialReportPackets();

  EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(10);
  for (int i = 0; i <= 9; ++i) {
    EXPECT_CALL(*sender(), OnReceiverCheckpoint(
                               FrameId::first() + i,
                               SimulatedFrame::GetExpectedPlayoutDelay(i)))
        .Times(1);
    EXPECT_CALL(*sender(), OnReceiverIsMissingPackets(_)).Times(0);

    sender()->SetFrameBeingSent(SimulatedFrame(start_time, i));
    // Send the frame's packets in-order half the time, out-of-order the other
    // half.
    const int permutation = (i % 2) ? i : 0;
    sender()->SendRtpPackets(sender()->GetAllPacketIds(permutation));

    AdvanceClockAndRunTasks(kRoundTripNetworkDelay);

    // The Receiver should immediately ACK once it has received all the RTP
    // packets to complete the frame.
    testing::Mock::VerifyAndClearExpectations(sender());

    // Advance to next frame transmission time.
    AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration -
                            kRoundTripNetworkDelay);
  }

  // When the Receiver has all of the frames and they are complete, it should
  // send out a low-frequency periodic RTCP "ping." Verify that there is one and
  // only one "ping" sent when the clock moves forward by one default report
  // interval during a period of inactivity.
  EXPECT_CALL(*sender(), OnReceiverCheckpoint(FrameId::first() + 9,
                                              kTargetPlayoutDelayChange))
      .Times(1);
  AdvanceClockAndRunTasks(kRtcpReportInterval);
  testing::Mock::VerifyAndClearExpectations(sender());

  ConsumeAndVerifyFrames(0, 9, start_time);
  EXPECT_EQ(Receiver::kNoFramesReady, receiver()->AdvanceToNextFrame());
}

// Tests that the Receiver processes RTP packets, can receive frames out of
// order, and issues the appropriate ACK/NACK feedback to the Sender as it
// realizes what it has and what it's missing.
TEST_F(ReceiverTest, ReceivesFramesOutOfOrder) {
  const Clock::time_point start_time = FakeClock::now();
  ExchangeInitialReportPackets();

  constexpr static int kOutOfOrderFrames[] = {3, 4, 2, 0, 1};
  for (int i : kOutOfOrderFrames) {
    // Expectations are different as each frame is sent and received.
    switch (i) {
      case 3: {
        // Note that frame 4 will not yet be known to the Receiver, and so it
        // should not be mentioned in any of the feedback for this case.
        EXPECT_CALL(*sender(), OnReceiverCheckpoint(FrameId::leader(),
                                                    kTargetPlayoutDelay))
            .Times(AtLeast(1));
        EXPECT_CALL(
            *sender(),
            OnReceiverHasFrames(std::vector<FrameId>({FrameId::first() + 3})))
            .Times(AtLeast(1));
        EXPECT_CALL(*sender(),
                    OnReceiverIsMissingPackets(std::vector<PacketNack>({
                        PacketNack{FrameId::first(), kAllPacketsLost},
                        PacketNack{FrameId::first() + 1, kAllPacketsLost},
                        PacketNack{FrameId::first() + 2, kAllPacketsLost},
                    })))
            .Times(AtLeast(1));
        EXPECT_CALL(*consumer(), OnFramesReady(_)).Times(0);
        break;
      }

      case 4: {
        EXPECT_CALL(*sender(), OnReceiverCheckpoint(FrameId::leader(),
                                                    kTargetPlayoutDelay))
            .Times(AtLeast(1));
        EXPECT_CALL(*sender(),
                    OnReceiverHasFrames(std::vector<FrameId>(
                        {FrameId::first() + 3, FrameId::first() + 4})))
            .Times(AtLeast(1));
        EXPECT_CALL(*sender(),
                    OnReceiverIsMissingPackets(std::vector<PacketNack>({
                        PacketNack{FrameId::first(), kAllPacketsLost},
                        PacketNack{FrameId::first() + 1, kAllPacketsLost},
                        PacketNack{FrameId::first() + 2, kAllPacketsLost},
                    })))
            .Times(AtLeast(1));
        EXPECT_CALL(*consumer(), OnFramesReady(_)).Times(0);
        break;
      }

      case 2: {
        EXPECT_CALL(*sender(), OnReceiverCheckpoint(FrameId::leader(),
                                                    kTargetPlayoutDelay))
            .Times(AtLeast(1));
        EXPECT_CALL(*sender(), OnReceiverHasFrames(std::vector<FrameId>(
                                   {FrameId::first() + 2, FrameId::first() + 3,
                                    FrameId::first() + 4})))
            .Times(AtLeast(1));
        EXPECT_CALL(*sender(),
                    OnReceiverIsMissingPackets(std::vector<PacketNack>({
                        PacketNack{FrameId::first(), kAllPacketsLost},
                        PacketNack{FrameId::first() + 1, kAllPacketsLost},
                    })))
            .Times(AtLeast(1));
        EXPECT_CALL(*consumer(), OnFramesReady(_)).Times(0);
        break;
      }

      case 0: {
        EXPECT_CALL(*sender(),
                    OnReceiverCheckpoint(FrameId::first(), kTargetPlayoutDelay))
            .Times(AtLeast(1));
        EXPECT_CALL(*sender(), OnReceiverHasFrames(std::vector<FrameId>(
                                   {FrameId::first() + 2, FrameId::first() + 3,
                                    FrameId::first() + 4})))
            .Times(AtLeast(1));
        EXPECT_CALL(*sender(),
                    OnReceiverIsMissingPackets(std::vector<PacketNack>(
                        {PacketNack{FrameId::first() + 1, kAllPacketsLost}})))
            .Times(AtLeast(1));
        EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(1);
        break;
      }

      case 1: {
        EXPECT_CALL(*sender(), OnReceiverCheckpoint(FrameId::first() + 4,
                                                    kTargetPlayoutDelay))
            .Times(AtLeast(1));
        EXPECT_CALL(*sender(), OnReceiverHasFrames(_)).Times(0);
        EXPECT_CALL(*sender(), OnReceiverIsMissingPackets(_)).Times(0);
        EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(1);
        break;
      }

      default:
        OSP_NOTREACHED();
    }

    sender()->SetFrameBeingSent(SimulatedFrame(start_time, i));
    sender()->SendRtpPackets(sender()->GetAllPacketIds(i));

    AdvanceClockAndRunTasks(kRoundTripNetworkDelay);

    // While there are known incomplete frames, the Receiver should send RTCP
    // packets more frequently than the default "ping" interval. Thus, advancing
    // the clock by this much should result in several feedback reports
    // transmitted to the Sender.
    AdvanceClockAndRunTasks(kRtcpReportInterval - kRoundTripNetworkDelay);

    testing::Mock::VerifyAndClearExpectations(sender());
    testing::Mock::VerifyAndClearExpectations(consumer());
  }

  ConsumeAndVerifyFrames(0, 4, start_time);
  EXPECT_EQ(Receiver::kNoFramesReady, receiver()->AdvanceToNextFrame());
}

// Tests that the Receiver will respond to a key frame request from its client
// by sending a Picture Loss Indicator (PLI) to the Sender, and then will
// automatically stop sending the PLI once a key frame has been received.
TEST_F(ReceiverTest, RequestsKeyFrameToRectifyPictureLoss) {
  const Clock::time_point start_time = FakeClock::now();
  ExchangeInitialReportPackets();

  // Send and Receive three frames in-order, normally.
  for (int i = 0; i <= 2; ++i) {
    EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(1);
    EXPECT_CALL(*sender(),
                OnReceiverCheckpoint(FrameId::first() + i, kTargetPlayoutDelay))
        .Times(1);
    sender()->SetFrameBeingSent(SimulatedFrame(start_time, i));
    sender()->SendRtpPackets(sender()->GetAllPacketIds(0));
    AdvanceClockAndRunTasks(kRoundTripNetworkDelay);
    testing::Mock::VerifyAndClearExpectations(sender());
    testing::Mock::VerifyAndClearExpectations(consumer());
    // Advance to next frame transmission time.
    AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration -
                            kRoundTripNetworkDelay);
  }
  ConsumeAndVerifyFrames(0, 2, start_time);

  // Simulate the Consumer requesting a key frame after picture loss (e.g., a
  // decoder failure). Ensure the Sender is immediately notified.
  EXPECT_CALL(*sender(), OnReceiverIndicatesPictureLoss()).Times(1);
  receiver()->RequestKeyFrame();
  AdvanceClockAndRunTasks(kOneWayNetworkDelay);  // Propagate request to Sender.
  testing::Mock::VerifyAndClearExpectations(sender());

  // The Sender sends another frame that is not a key frame and, upon receipt,
  // the Receiver should repeat its "cry" for a key frame.
  EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(1);
  EXPECT_CALL(*sender(),
              OnReceiverCheckpoint(FrameId::first() + 3, kTargetPlayoutDelay))
      .Times(1);
  EXPECT_CALL(*sender(), OnReceiverIndicatesPictureLoss()).Times(AtLeast(1));
  sender()->SetFrameBeingSent(SimulatedFrame(start_time, 3));
  sender()->SendRtpPackets(sender()->GetAllPacketIds(0));
  AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration - kOneWayNetworkDelay);
  testing::Mock::VerifyAndClearExpectations(sender());
  testing::Mock::VerifyAndClearExpectations(consumer());
  ConsumeAndVerifyFrames(3, 3, start_time);

  // Finally, the Sender responds to the PLI condition by sending a key frame.
  // Confirm the Receiver has stopped indicating picture loss after having
  // received the key frame.
  EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(1);
  EXPECT_CALL(*sender(),
              OnReceiverCheckpoint(FrameId::first() + 4, kTargetPlayoutDelay))
      .Times(1);
  EXPECT_CALL(*sender(), OnReceiverIndicatesPictureLoss()).Times(0);
  SimulatedFrame key_frame(start_time, 4);
  key_frame.dependency = EncodedFrame::Dependency::kKeyFrame;
  key_frame.referenced_frame_id = key_frame.frame_id;
  sender()->SetFrameBeingSent(key_frame);
  sender()->SendRtpPackets(sender()->GetAllPacketIds(0));
  AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration);
  testing::Mock::VerifyAndClearExpectations(sender());
  testing::Mock::VerifyAndClearExpectations(consumer());

  // The client has not yet consumed the key frame, so any calls to
  // RequestKeyFrame() should not set the PLI condition again.
  EXPECT_CALL(*sender(), OnReceiverIndicatesPictureLoss()).Times(0);
  receiver()->RequestKeyFrame();
  AdvanceClockAndRunTasks(kOneWayNetworkDelay);
  testing::Mock::VerifyAndClearExpectations(sender());

  // After consuming the requested key frame, the client should be able to set
  // the PLI condition again with another RequestKeyFrame() call.
  ConsumeAndVerifyFrame(key_frame);
  EXPECT_CALL(*sender(), OnReceiverIndicatesPictureLoss()).Times(1);
  receiver()->RequestKeyFrame();
  AdvanceClockAndRunTasks(kOneWayNetworkDelay);
  testing::Mock::VerifyAndClearExpectations(sender());
}

TEST_F(ReceiverTest, PLICanBeDisabled) {
  receiver()->SetPliEnabledForTesting(false);

#if OSP_DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(receiver()->RequestKeyFrame(),
                            ".*PLI is not enabled.*");
#else
  EXPECT_CALL(*sender(), OnReceiverIndicatesPictureLoss()).Times(0);
  receiver()->RequestKeyFrame();
  AdvanceClockAndRunTasks(kOneWayNetworkDelay);
  testing::Mock::VerifyAndClearExpectations(sender());
#endif
}

// Tests that the Receiver will start dropping packets once its frame queue is
// full (i.e., when the consumer is not pulling them out of the queue). Since
// the Receiver will stop ACK'ing frames, the Sender will become stalled.
TEST_F(ReceiverTest, EatsItsFill) {
  const Clock::time_point start_time = FakeClock::now();
  ExchangeInitialReportPackets();

  // Send and Receive the maximum possible number of frames in-order, normally.
  for (int i = 0; i < kMaxUnackedFrames; ++i) {
    EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(1);
    EXPECT_CALL(*sender(), OnReceiverCheckpoint(
                               FrameId::first() + i,
                               SimulatedFrame::GetExpectedPlayoutDelay(i)))
        .Times(1);
    sender()->SetFrameBeingSent(SimulatedFrame(start_time, i));
    sender()->SendRtpPackets(sender()->GetAllPacketIds(0));
    AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration);
    testing::Mock::VerifyAndClearExpectations(sender());
    testing::Mock::VerifyAndClearExpectations(consumer());
  }

  // Sending one more frame should be ignored. Over and over. None of the
  // feedback reports from the Receiver should indicate it is collecting packets
  // for future frames.
  int ignored_frame = kMaxUnackedFrames;
  for (int i = 0; i < 5; ++i) {
    EXPECT_CALL(*consumer(), OnFramesReady(_)).Times(0);
    EXPECT_CALL(*sender(),
                OnReceiverCheckpoint(FrameId::first() + (ignored_frame - 1),
                                     kTargetPlayoutDelayChange))
        .Times(AtLeast(0));
    EXPECT_CALL(*sender(), OnReceiverIsMissingPackets(_)).Times(0);
    sender()->SetFrameBeingSent(SimulatedFrame(start_time, ignored_frame));
    sender()->SendRtpPackets(sender()->GetAllPacketIds(0));
    AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration);
    testing::Mock::VerifyAndClearExpectations(sender());
    testing::Mock::VerifyAndClearExpectations(consumer());
  }

  // Consume only one frame, and confirm the Receiver allows only one frame more
  // to be received.
  ConsumeAndVerifyFrames(0, 0, start_time);
  int no_longer_ignored_frame = ignored_frame;
  ++ignored_frame;
  EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(AtLeast(1));
  EXPECT_CALL(*sender(),
              OnReceiverCheckpoint(FrameId::first() + no_longer_ignored_frame,
                                   kTargetPlayoutDelayChange))
      .Times(AtLeast(1));
  EXPECT_CALL(*sender(), OnReceiverIsMissingPackets(_)).Times(0);
  // This frame should be received successfully.
  sender()->SetFrameBeingSent(
      SimulatedFrame(start_time, no_longer_ignored_frame));
  sender()->SendRtpPackets(sender()->GetAllPacketIds(0));
  AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration);
  // This second frame should be ignored, however.
  sender()->SetFrameBeingSent(SimulatedFrame(start_time, ignored_frame));
  sender()->SendRtpPackets(sender()->GetAllPacketIds(0));
  AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration);
  testing::Mock::VerifyAndClearExpectations(sender());
  testing::Mock::VerifyAndClearExpectations(consumer());
}

// Tests that incomplete frames that would be played-out too late are dropped,
// but only as inter-frame data dependency requirements permit, and only if no
// target playout delay change information would have been missed.
TEST_F(ReceiverTest, DropsLateFrames) {
  const Clock::time_point start_time = FakeClock::now();
  ExchangeInitialReportPackets();

  // Before any packets have been sent/received, the Receiver should indicate no
  // frames are ready.
  EXPECT_EQ(Receiver::kNoFramesReady, receiver()->AdvanceToNextFrame());

  // Set a ridiculously-large estimated player processing time so that the logic
  // thinks every frame going to play out too late.
  receiver()->SetPlayerProcessingTime(seconds(3));

  // In this test there are eight frames total:
  //   - Frame 0: Key frame.
  //   - Frames 1-4: Non-key frames.
  //   - Frame 5: Non-key frame that contains a target playout delay change.
  //   - Frame 6: Key frame.
  //   - Frame 7: Non-key frame.
  ASSERT_EQ(SimulatedFrame::kPlayoutChangeAtFrame, 5);
  SimulatedFrame frames[8] = {{start_time, 0}, {start_time, 1}, {start_time, 2},
                              {start_time, 3}, {start_time, 4}, {start_time, 5},
                              {start_time, 6}, {start_time, 7}};
  frames[6].dependency = EncodedFrame::Dependency::kKeyFrame;
  frames[6].referenced_frame_id = frames[6].frame_id;

  // Send just packet 1 (NOT packet 0) of all the frames. The Receiver should
  // never notify the consumer via the callback, nor report that any frames are
  // ready, because none of the frames have been completely received.
  EXPECT_CALL(*consumer(), OnFramesReady(_)).Times(0);
  EXPECT_CALL(*sender(), OnReceiverCheckpoint(_, _)).Times(0);
  for (int i = 0; i <= 7; ++i) {
    sender()->SetFrameBeingSent(frames[i]);
    // Assumption: There are at least three packets in each frame, else the test
    // is not exercising the logic meaningfully.
    ASSERT_LE(size_t{3}, sender()->GetAllPacketIds(0).size());
    sender()->SendRtpPackets({FramePacketId{1}});
    AdvanceClockAndRunTasks(SimulatedFrame::kFrameDuration);
  }
  testing::Mock::VerifyAndClearExpectations(consumer());
  testing::Mock::VerifyAndClearExpectations(sender());
  EXPECT_EQ(Receiver::kNoFramesReady, receiver()->AdvanceToNextFrame());

  // Send all the packets of Frame 6 (the second key frame) and Frame 7. The
  // Receiver still cannot drop any frames because it has not seen packet 0 of
  // every prior frame. In other words, it cannot ignore any possibility of a
  // target playout delay change from the Sender.
  EXPECT_CALL(*consumer(), OnFramesReady(_)).Times(0);
  EXPECT_CALL(*sender(), OnReceiverCheckpoint(_, _)).Times(0);
  for (int i = 6; i <= 7; ++i) {
    sender()->SetFrameBeingSent(frames[i]);
    sender()->SendRtpPackets(sender()->GetAllPacketIds(0));
  }
  AdvanceClockAndRunTasks(kRoundTripNetworkDelay);
  testing::Mock::VerifyAndClearExpectations(consumer());
  testing::Mock::VerifyAndClearExpectations(sender());
  EXPECT_EQ(Receiver::kNoFramesReady, receiver()->AdvanceToNextFrame());

  // Send packet 0 for all but Frame 5, which contains a target playout delay
  // change. All but the last two frames will still be incomplete. The Receiver
  // still cannot drop any frames because it doesn't know whether Frame 5 had a
  // target playout delay change.
  EXPECT_CALL(*consumer(), OnFramesReady(_)).Times(0);
  EXPECT_CALL(*sender(), OnReceiverCheckpoint(_, _)).Times(0);
  for (int i = 0; i <= 7; ++i) {
    if (i == 5) {
      continue;
    }
    sender()->SetFrameBeingSent(frames[i]);
    sender()->SendRtpPackets({FramePacketId{0}});
  }
  AdvanceClockAndRunTasks(kRoundTripNetworkDelay);
  testing::Mock::VerifyAndClearExpectations(consumer());
  testing::Mock::VerifyAndClearExpectations(sender());
  EXPECT_EQ(Receiver::kNoFramesReady, receiver()->AdvanceToNextFrame());

  // Finally, send packet 0 for Frame 5. Now, the Receiver will drop every frame
  // before the completely-received second key frame, as they are all still
  // incomplete and will play-out too late. When it drops the frames, it will
  // notify the sender of the new checkpoint so that it stops trying to
  // re-transmit the dropped frames.
  EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(1);
  EXPECT_CALL(*sender(), OnReceiverCheckpoint(FrameId::first() + 7,
                                              kTargetPlayoutDelayChange))
      .Times(1);
  sender()->SetFrameBeingSent(frames[5]);
  sender()->SendRtpPackets({FramePacketId{0}});
  AdvanceClockAndRunTasks(kRoundTripNetworkDelay);
  // Note: Consuming Frame 6 will trigger the checkpoint advancement, since the
  // call to AdvanceToNextFrame() contains the frame skipping/dropping logic.
  ConsumeAndVerifyFrame(frames[6]);
  testing::Mock::VerifyAndClearExpectations(consumer());
  testing::Mock::VerifyAndClearExpectations(sender());

  // After consuming Frame 6, the Receiver knows Frame 7 is also available and
  // should have scheduled an immediate task to notify the Consumer of this.
  EXPECT_CALL(*consumer(), OnFramesReady(Gt(0))).Times(1);
  AdvanceClockAndRunTasks(kOneWayNetworkDelay);
  testing::Mock::VerifyAndClearExpectations(consumer());

  // Now consume Frame 7. This shouldn't trigger any further checkpoint
  // advancement.
  EXPECT_CALL(*consumer(), OnFramesReady(_)).Times(0);
  EXPECT_CALL(*sender(), OnReceiverCheckpoint(_, _)).Times(0);
  ConsumeAndVerifyFrame(frames[7]);
  AdvanceClockAndRunTasks(kOneWayNetworkDelay);
  testing::Mock::VerifyAndClearExpectations(consumer());
  testing::Mock::VerifyAndClearExpectations(sender());
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
