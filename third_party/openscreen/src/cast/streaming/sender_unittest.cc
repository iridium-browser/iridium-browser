// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/sender.h"

#include <stdint.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "cast/streaming/compound_rtcp_builder.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/encoded_frame.h"
#include "cast/streaming/frame_collector.h"
#include "cast/streaming/frame_crypto.h"
#include "cast/streaming/frame_id.h"
#include "cast/streaming/mock_environment.h"
#include "cast/streaming/packet_util.h"
#include "cast/streaming/rtcp_session.h"
#include "cast/streaming/rtp_defines.h"
#include "cast/streaming/rtp_packet_parser.h"
#include "cast/streaming/sender_packet_router.h"
#include "cast/streaming/sender_report_parser.h"
#include "cast/streaming/session_config.h"
#include "cast/streaming/ssrc.h"
#include "cast/streaming/testing/simple_socket_subscriber.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/base/span.h"
#include "platform/test/byte_view_test_util.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "util/alarm.h"
#include "util/chrono_helpers.h"
#include "util/std_util.h"
#include "util/yet_another_bit_vector.h"

using testing::_;
using testing::AtLeast;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::Sequence;

namespace openscreen {
namespace cast {
namespace {

// Sender configuration.
constexpr Ssrc kSenderSsrc = 1;
constexpr Ssrc kReceiverSsrc = 2;
constexpr int kRtpTimebase = 48000;
constexpr milliseconds kTargetPlayoutDelay{400};
constexpr auto kAesKey =
    std::array<uint8_t, 16>{{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f}};
constexpr auto kCastIvMask =
    std::array<uint8_t, 16>{{0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80,
                             0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00}};
constexpr RtpPayloadType kRtpPayloadType = RtpPayloadType::kVideoVp8;

// The number of RTP ticks advanced per frame, for 100 FPS media.
constexpr int kRtpTicksPerFrame = kRtpTimebase / 100;

// The number of milliseconds advanced per frame, for 100 FPS media.
constexpr milliseconds kFrameDuration{1000 / 100};
static_assert(kFrameDuration < (kTargetPlayoutDelay / 10),
              "Kickstart test assumes frame duration is far less than the "
              "playout delay.");

// An Encoded frame that also holds onto its own copy of data.
struct EncodedFrameWithBuffer : public EncodedFrame {
  // |EncodedFrame::data| always points inside buffer.begin()...buffer.end().
  std::vector<uint8_t> buffer;
};

// SenderPacketRouter configuration for these tests.
constexpr int kNumPacketsPerBurst = 20;
constexpr milliseconds kBurstInterval{10};

// An arbitrary value, subtracted from "now," to specify the reference_time on
// frames that are about to be enqueued. This simulates that capture+encode
// happened in the past, before Sender::EnqueueFrame() is called.
constexpr milliseconds kCaptureDelay{11};

// In some tests, the computed time values could be off a little bit due to
// imprecision in certain wire-format timestamp types. The following macro
// behaves just like Gtest's EXPECT_NEAR(), but works with all the time types
// too.
#define EXPECT_NEARLY_EQUAL(duration_a, duration_b, epsilon) \
  if ((duration_a) >= (duration_b)) {                        \
    EXPECT_LE((duration_a), (duration_b) + (epsilon));       \
  } else {                                                   \
    EXPECT_GE((duration_a), (duration_b) - (epsilon));       \
  }

void OverrideRtpTimestamp(int frame_count, EncodedFrame* frame, int fps) {
  const int ticks = frame_count * kRtpTimebase / fps;
  frame->rtp_timestamp = RtpTimeTicks() + RtpTimeDelta::FromTicks(ticks);
}

// Simulates UDP/IPv6 traffic in one direction (from Sender→Receiver, or
// Receiver→Sender), with a settable amount of delay.
class SimulatedNetworkPipe {
 public:
  SimulatedNetworkPipe(TaskRunner* task_runner,
                       Environment::PacketConsumer* remote)
      : task_runner_(task_runner), remote_(remote) {
    // Create a fake IPv6 address using the "documentative purposes" prefix
    // concatenated with the |this| pointer.
    std::array<uint16_t, 8> hextets{};
    hextets[0] = 0x2001;
    hextets[1] = 0x0db8;
    auto* const this_pointer = this;
    static_assert(sizeof(this_pointer) <= (6 * sizeof(uint16_t)), "");
    memcpy(&hextets[2], &this_pointer, sizeof(this_pointer));
    local_endpoint_ = IPEndpoint{IPAddress(hextets), 2344};
  }

  const IPEndpoint& local_endpoint() const { return local_endpoint_; }

  Clock::duration network_delay() const { return network_delay_; }
  void set_network_delay(Clock::duration delay) { network_delay_ = delay; }

  // The caller needs to spin the task runner before |packet| will reach the
  // other side.
  void StartPacketTransmission(std::vector<uint8_t> packet) {
    task_runner_->PostTaskWithDelay(
        [this, packet = std::move(packet)]() mutable {
          remote_->OnReceivedPacket(local_endpoint_, FakeClock::now(),
                                    std::move(packet));
        },
        network_delay_);
  }

 private:
  TaskRunner* const task_runner_;
  Environment::PacketConsumer* const remote_;

  IPEndpoint local_endpoint_;

  // The amount of time for the packet to transmit over this simulated network
  // pipe. Defaults to zero to simplify the tests that don't care about delays.
  Clock::duration network_delay_{};
};

// Processes packets from the Sender under test, allowing unit tests to set
// expectations for parsed RTP or RTCP packets, to confirm proper behavior of
// the Sender.
class MockReceiver : public Environment::PacketConsumer {
 public:
  explicit MockReceiver(SimulatedNetworkPipe* pipe_to_sender)
      : pipe_to_sender_(pipe_to_sender),
        rtcp_session_(kSenderSsrc, kReceiverSsrc, FakeClock::now()),
        sender_report_parser_(&rtcp_session_),
        rtcp_builder_(&rtcp_session_),
        rtp_parser_(kSenderSsrc),
        crypto_(kAesKey, kCastIvMask) {
    rtcp_builder_.SetPlayoutDelay(kTargetPlayoutDelay);
  }

  ~MockReceiver() override = default;

  // Simulate the Receiver ACK'ing all frames up to and including the
  // |new_checkpoint|.
  void SetCheckpointFrame(FrameId new_checkpoint) {
    OSP_CHECK_GE(new_checkpoint, rtcp_builder_.checkpoint_frame());
    rtcp_builder_.SetCheckpointFrame(new_checkpoint);
  }

  // Automatically advances the checkpoint based on what is found in
  // |complete_frames_|, returning true if the checkpoint moved forward.
  bool AutoAdvanceCheckpoint() {
    const FrameId old_checkpoint = rtcp_builder_.checkpoint_frame();
    FrameId new_checkpoint = old_checkpoint;
    for (auto it = complete_frames_.upper_bound(old_checkpoint);
         it != complete_frames_.end(); ++it) {
      if (it->first != new_checkpoint + 1) {
        break;
      }
      ++new_checkpoint;
    }
    if (new_checkpoint > old_checkpoint) {
      rtcp_builder_.SetCheckpointFrame(new_checkpoint);
      return true;
    }
    return false;
  }

  void SetPictureLossIndicator(bool picture_is_lost) {
    rtcp_builder_.SetPictureLossIndicator(picture_is_lost);
  }

  void SetReceiverReport(StatusReportId reply_for,
                         RtcpReportBlock::Delay processing_delay) {
    RtcpReportBlock receiver_report;
    receiver_report.ssrc = kSenderSsrc;
    receiver_report.last_status_report_id = reply_for;
    receiver_report.delay_since_last_report = processing_delay;
    rtcp_builder_.IncludeReceiverReportInNextPacket(receiver_report);
  }

  void SetNacksAndAcks(std::vector<PacketNack> packet_nacks,
                       std::vector<FrameId> frame_acks) {
    rtcp_builder_.IncludeFeedbackInNextPacket(std::move(packet_nacks),
                                              std::move(frame_acks));
  }

  // Builds and sends a RTCP packet containing one or more of: checkpoint, PLI,
  // Receiver Report, NACKs, ACKs.
  void TransmitRtcpFeedbackPacket() {
    uint8_t buffer[kMaxRtpPacketSizeForIpv6UdpOnEthernet];
    const absl::Span<uint8_t> packet =
        rtcp_builder_.BuildPacket(FakeClock::now(), buffer);
    pipe_to_sender_->StartPacketTransmission(
        std::vector<uint8_t>(packet.begin(), packet.end()));
  }

  // Used by tests to simulate the Receiver not seeing specific packets come in
  // from the network (e.g., because the network dropped the packets).
  void SetIgnoreList(std::vector<PacketNack> ignore_list) {
    ignore_list_ = ignore_list;
  }

  // Environment::PacketConsumer implementation.
  //
  // Called to process a packet from the Sender, simulating basic RTP frame
  // collection and Sender Report parsing/handling.
  void OnReceivedPacket(const IPEndpoint& source,
                        Clock::time_point arrival_time,
                        std::vector<uint8_t> packet) override {
    const auto type_and_ssrc = InspectPacketForRouting(packet);
    EXPECT_NE(ApparentPacketType::UNKNOWN, type_and_ssrc.first);
    EXPECT_EQ(kSenderSsrc, type_and_ssrc.second);
    if (type_and_ssrc.first == ApparentPacketType::RTP) {
      const absl::optional<RtpPacketParser::ParseResult> part_of_frame =
          rtp_parser_.Parse(packet);
      ASSERT_TRUE(part_of_frame);

      // Return early if simulating packet drops over the network.
      if (ContainsIf(ignore_list_, [&](const PacketNack& baddie) {
            return (baddie.frame_id == part_of_frame->frame_id &&
                    (baddie.packet_id == kAllPacketsLost ||
                     baddie.packet_id == part_of_frame->packet_id));
          })) {
        return;
      }

      OnRtpPacket(*part_of_frame);
      CollectRtpPacket(*part_of_frame, std::move(packet));
    } else if (type_and_ssrc.first == ApparentPacketType::RTCP) {
      absl::optional<SenderReportParser::SenderReportWithId> report =
          sender_report_parser_.Parse(packet);
      ASSERT_TRUE(report);
      OnSenderReport(*report);
    }
  }

  std::map<FrameId, EncodedFrameWithBuffer> TakeCompleteFrames() {
    std::map<FrameId, EncodedFrameWithBuffer> result;
    result.swap(complete_frames_);
    return result;
  }

  // Tests set expectations on these mocks to monitor events of interest, and/or
  // invoke additional behaviors.
  MOCK_METHOD1(OnRtpPacket,
               void(const RtpPacketParser::ParseResult& parsed_packet));
  MOCK_METHOD1(OnFrameComplete, void(FrameId frame_id));
  MOCK_METHOD1(OnSenderReport,
               void(const SenderReportParser::SenderReportWithId& report));

 private:
  // Collects the individual RTP packets until a whole frame can be formed, then
  // calls OnFrameComplete(). Ignores extra RTP packets that are no longer
  // needed.
  void CollectRtpPacket(const RtpPacketParser::ParseResult& part_of_frame,
                        std::vector<uint8_t> packet) {
    const FrameId frame_id = part_of_frame.frame_id;
    if (complete_frames_.find(frame_id) != complete_frames_.end()) {
      return;
    }
    FrameCollector& collector = incomplete_frames_[frame_id];
    collector.set_frame_id(frame_id);
    EXPECT_TRUE(collector.CollectRtpPacket(part_of_frame, &packet));
    if (!collector.is_complete()) {
      return;
    }
    const EncryptedFrame& encrypted = collector.PeekAtAssembledFrame();
    EncodedFrameWithBuffer* const decrypted = &complete_frames_[frame_id];
    // Note: Not setting decrypted->reference_time here since the logic around
    // calculating the playout time is rather complex, and is definitely outside
    // the scope of the testing being done in this module. Instead, end-to-end
    // testing should exist elsewhere to confirm frame play-out times with real
    // Receivers.
    decrypted->buffer.resize(FrameCrypto::GetPlaintextSize(encrypted));
    crypto_.Decrypt(encrypted, decrypted->buffer);
    encrypted.CopyMetadataTo(decrypted);
    decrypted->data = decrypted->buffer;
    incomplete_frames_.erase(frame_id);
    OnFrameComplete(frame_id);
  }

  SimulatedNetworkPipe* const pipe_to_sender_;
  RtcpSession rtcp_session_;
  SenderReportParser sender_report_parser_;
  CompoundRtcpBuilder rtcp_builder_;
  RtpPacketParser rtp_parser_;
  FrameCrypto crypto_;

  std::vector<PacketNack> ignore_list_;
  std::map<FrameId, FrameCollector> incomplete_frames_;
  std::map<FrameId, EncodedFrameWithBuffer> complete_frames_;
};

class MockObserver : public Sender::Observer {
 public:
  MOCK_METHOD1(OnFrameCanceled, void(FrameId frame_id));
  MOCK_METHOD0(OnPictureLost, void());
};

class SenderTest : public testing::Test {
 public:
  SenderTest()
      : fake_clock_(Clock::now()),
        task_runner_(&fake_clock_),
        sender_environment_(&FakeClock::now, &task_runner_),
        sender_packet_router_(&sender_environment_,
                              kNumPacketsPerBurst,
                              kBurstInterval),
        sender_(&sender_environment_,
                &sender_packet_router_,
                {/* .sender_ssrc = */ kSenderSsrc,
                 /* .receiver_ssrc = */ kReceiverSsrc,
                 /* .rtp_timebase = */ kRtpTimebase,
                 /* .channels = */ 2,
                 /* .target_playout_delay = */ kTargetPlayoutDelay,
                 /* .aes_secret_key = */ kAesKey,
                 /* .aes_iv_mask = */ kCastIvMask,
                 /* .is_pli_enabled = */ true},
                kRtpPayloadType),
        receiver_to_sender_pipe_(&task_runner_, &sender_packet_router_),
        receiver_(&receiver_to_sender_pipe_),
        sender_to_receiver_pipe_(&task_runner_, &receiver_) {
    sender_environment_.SetSocketSubscriber(&socket_subscriber_);
    sender_environment_.set_remote_endpoint(
        receiver_to_sender_pipe_.local_endpoint());
    ON_CALL(sender_environment_, SendPacket(_))
        .WillByDefault(Invoke([this](absl::Span<const uint8_t> packet) {
          sender_to_receiver_pipe_.StartPacketTransmission(
              std::vector<uint8_t>(packet.begin(), packet.end()));
        }));
  }

  ~SenderTest() override = default;

  Sender* sender() { return &sender_; }
  MockReceiver* receiver() { return &receiver_; }

  void SetReceiverToSenderNetworkDelay(Clock::duration delay) {
    receiver_to_sender_pipe_.set_network_delay(delay);
  }

  void SetSenderToReceiverNetworkDelay(Clock::duration delay) {
    sender_to_receiver_pipe_.set_network_delay(delay);
  }

  void SimulateExecution(Clock::duration how_long = Clock::duration::zero()) {
    fake_clock_.Advance(how_long);
  }

  static void PopulateFramePayloadBuffer(int seed,
                                         int num_bytes,
                                         std::vector<uint8_t>* payload) {
    payload->clear();
    payload->reserve(num_bytes);
    for (int i = 0; i < num_bytes; ++i) {
      payload->push_back(static_cast<uint8_t>(seed + i));
    }
  }

  static void PopulateFrameWithDefaults(FrameId frame_id,
                                        Clock::time_point reference_time,
                                        int seed,
                                        int num_payload_bytes,
                                        EncodedFrameWithBuffer* frame) {
    frame->dependency = (frame_id == FrameId::first())
                            ? EncodedFrame::Dependency::kKeyFrame
                            : EncodedFrame::Dependency::kDependent;
    frame->frame_id = frame_id;
    frame->referenced_frame_id = frame->frame_id;
    if (frame_id != FrameId::first()) {
      --frame->referenced_frame_id;
    }
    frame->rtp_timestamp =
        RtpTimeTicks() + (RtpTimeDelta::FromTicks(kRtpTicksPerFrame) *
                          (frame_id - FrameId::first()));
    frame->reference_time = reference_time;
    PopulateFramePayloadBuffer(seed, num_payload_bytes, &frame->buffer);
    frame->data = frame->buffer;
  }

  // Confirms that all |sent_frames| exist in |received_frames|, with identical
  // data and metadata.
  static void ExpectFramesReceivedCorrectly(
      absl::Span<EncodedFrameWithBuffer> sent_frames,
      const std::map<FrameId, EncodedFrameWithBuffer> received_frames) {
    ASSERT_EQ(sent_frames.size(), received_frames.size());

    for (const EncodedFrameWithBuffer& sent_frame : sent_frames) {
      SCOPED_TRACE(testing::Message()
                   << "Checking sent frame " << sent_frame.frame_id);
      const auto received_it = received_frames.find(sent_frame.frame_id);
      if (received_it == received_frames.end()) {
        ADD_FAILURE() << "Did not receive frame.";
        continue;
      }
      const EncodedFrame& received_frame = received_it->second;
      EXPECT_EQ(sent_frame.dependency, received_frame.dependency);
      EXPECT_EQ(sent_frame.referenced_frame_id,
                received_frame.referenced_frame_id);
      EXPECT_EQ(sent_frame.rtp_timestamp, received_frame.rtp_timestamp);
      ExpectByteViewsHaveSameBytes(sent_frame.data, received_frame.data);
    }
  }

 private:
  FakeClock fake_clock_;
  FakeTaskRunner task_runner_;
  NiceMock<MockEnvironment> sender_environment_;
  SenderPacketRouter sender_packet_router_;
  Sender sender_;
  SimulatedNetworkPipe receiver_to_sender_pipe_;
  NiceMock<MockReceiver> receiver_;
  SimulatedNetworkPipe sender_to_receiver_pipe_;
  SimpleSubscriber socket_subscriber_;
};

// Tests that the Sender can send EncodedFrames over an ideal network (i.e., low
// latency, no loss), and does so without having to transmit the same packet
// twice.
TEST_F(SenderTest, SendsFramesEfficiently) {
  constexpr milliseconds kOneWayNetworkDelay{1};
  SetSenderToReceiverNetworkDelay(kOneWayNetworkDelay);
  SetReceiverToSenderNetworkDelay(kOneWayNetworkDelay);

  // Expect that each packet is only sent once.
  std::set<std::pair<FrameId, FramePacketId>> received_packets;
  EXPECT_CALL(*receiver(), OnRtpPacket(_))
      .WillRepeatedly(
          Invoke([&](const RtpPacketParser::ParseResult& parsed_packet) {
            std::pair<FrameId, FramePacketId> id(parsed_packet.frame_id,
                                                 parsed_packet.packet_id);
            const auto insert_result = received_packets.insert(id);
            EXPECT_TRUE(insert_result.second)
                << "Received duplicate packet: " << id.first << ':'
                << static_cast<int>(id.second);
          }));

  // Simulate normal frame ACK'ing behavior.
  ON_CALL(*receiver(), OnFrameComplete(_)).WillByDefault(InvokeWithoutArgs([&] {
    if (receiver()->AutoAdvanceCheckpoint()) {
      receiver()->TransmitRtcpFeedbackPacket();
    }
  }));

  NiceMock<MockObserver> observer;
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first())).Times(1);
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first() + 1)).Times(1);
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first() + 2)).Times(1);
  sender()->SetObserver(&observer);

  EncodedFrameWithBuffer frames[3];
  constexpr int kFrameDataSizes[] = {8196, 12, 1900};
  for (int i = 0; i < 3; ++i) {
    if (i == 0) {
      EXPECT_TRUE(sender()->NeedsKeyFrame());
    } else {
      EXPECT_FALSE(sender()->NeedsKeyFrame());
    }
    PopulateFrameWithDefaults(FrameId::first() + i,
                              FakeClock::now() - kCaptureDelay, 0xbf - i,
                              kFrameDataSizes[i], &frames[i]);
    ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frames[i]));
    SimulateExecution(kFrameDuration);
  }
  SimulateExecution(kTargetPlayoutDelay);

  ExpectFramesReceivedCorrectly(frames, receiver()->TakeCompleteFrames());
}

// Tests that the Sender correctly computes the current in-flight media
// duration, a backlog signal for clients.
TEST_F(SenderTest, ComputesInFlightMediaDuration) {
  // With no frames enqueued, the in-flight media duration should be zero.
  EXPECT_EQ(Clock::duration::zero(),
            sender()->GetInFlightMediaDuration(RtpTimeTicks()));
  EXPECT_EQ(Clock::duration::zero(),
            sender()->GetInFlightMediaDuration(
                RtpTimeTicks() + RtpTimeDelta::FromTicks(kRtpTicksPerFrame)));

  // Enqueue a frame.
  EncodedFrameWithBuffer frame;
  PopulateFrameWithDefaults(FrameId::first(), FakeClock::now(), 0,
                            13 /* bytes */, &frame);
  ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frame));

  // Now, the in-flight media duration should depend on the RTP timestamp of the
  // next frame.
  EXPECT_EQ(kFrameDuration, sender()->GetInFlightMediaDuration(
                                frame.rtp_timestamp +
                                RtpTimeDelta::FromTicks(kRtpTicksPerFrame)));
  EXPECT_EQ(10 * kFrameDuration,
            sender()->GetInFlightMediaDuration(
                frame.rtp_timestamp +
                RtpTimeDelta::FromTicks(10 * kRtpTicksPerFrame)));
}

// Tests that the Sender computes the maximum in-flight media duration based on
// its analysis of current network conditions. By implication, this demonstrates
// that the Sender is also measuring the network round-trip time.
TEST_F(SenderTest, RespondsToNetworkLatencyChanges) {
  // The expected maximum error in time calculations is one tick of the RTCP
  // report block's delay type.
  constexpr auto kEpsilon = to_nanoseconds(RtcpReportBlock::Delay(1));

  // Before the Sender has the necessary information to compute the network
  // round-trip time, GetMaxInFlightMediaDuration() will return half the target
  // playout delay.
  EXPECT_NEARLY_EQUAL(kTargetPlayoutDelay / 2,
                      sender()->GetMaxInFlightMediaDuration(), kEpsilon);

  // No network is perfect. Simulate different one-way network delays.
  constexpr milliseconds kOutboundDelay{2};
  constexpr milliseconds kInboundDelay{4};
  constexpr milliseconds kRoundTripDelay = kOutboundDelay + kInboundDelay;
  SetSenderToReceiverNetworkDelay(kOutboundDelay);
  SetReceiverToSenderNetworkDelay(kInboundDelay);

  // Enqueue a frame in the Sender to start emitting periodic RTCP reports.
  {
    EncodedFrameWithBuffer frame;
    PopulateFrameWithDefaults(FrameId::first(), FakeClock::now(), 0,
                              1 /* byte */, &frame);
    ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frame));
  }

  // Run one network round-trip from Sender→Receiver→Sender.
  StatusReportId sender_report_id{};
  EXPECT_CALL(*receiver(), OnSenderReport(_))
      .WillOnce(Invoke(
          [&](const SenderReportParser::SenderReportWithId& sender_report) {
            sender_report_id = sender_report.report_id;
          }));
  // Simulate the passage of time for the Sender Report to reach the Receiver.
  SimulateExecution(kOutboundDelay);
  // The Receiver should have received the Sender Report at this point.
  Mock::VerifyAndClearExpectations(receiver());
  ASSERT_NE(StatusReportId{}, sender_report_id);
  // Simulate the passage of time in the Receiver doing "other tasks" before
  // replying back to the Sender. This delay is included in the Receiver Report
  // so that the Sender can isolate the delays caused by the network.
  constexpr milliseconds kReceiverProcessingDelay{2};
  SimulateExecution(kReceiverProcessingDelay);
  // Create the Receiver Report "reply," and simulate it being sent across the
  // network, back to the Sender.
  receiver()->SetReceiverReport(
      sender_report_id, std::chrono::duration_cast<RtcpReportBlock::Delay>(
                            kReceiverProcessingDelay));
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution(kInboundDelay);

  // At this point, the Sender should have computed the network round-trip time,
  // and so GetMaxInFlightMediaDuration() will return half the target playout
  // delay PLUS half the network round-trip time.
  EXPECT_NEARLY_EQUAL(kTargetPlayoutDelay / 2 + kRoundTripDelay / 2,
                      sender()->GetMaxInFlightMediaDuration(), kEpsilon);

  // Increase the outbound delay, which will increase the total round-trip time.
  constexpr milliseconds kIncreasedOutboundDelay{6};
  constexpr milliseconds kIncreasedRoundTripDelay =
      kIncreasedOutboundDelay + kInboundDelay;
  SetSenderToReceiverNetworkDelay(kIncreasedOutboundDelay);

  // With increased network delay, run several more network round-trips. Expect
  // the Sender to gradually converge towards the new network round-trip time.
  constexpr int kNumReportIntervals = 50;
  EXPECT_CALL(*receiver(), OnSenderReport(_))
      .Times(kNumReportIntervals)
      .WillRepeatedly(Invoke(
          [&](const SenderReportParser::SenderReportWithId& sender_report) {
            receiver()->SetReceiverReport(sender_report.report_id,
                                          RtcpReportBlock::Delay::zero());
            receiver()->TransmitRtcpFeedbackPacket();
          }));
  Clock::duration last_max = sender()->GetMaxInFlightMediaDuration();
  for (int i = 0; i < kNumReportIntervals; ++i) {
    SimulateExecution(kRtcpReportInterval);
    const Clock::duration updated_value =
        sender()->GetMaxInFlightMediaDuration();
    EXPECT_LE(last_max, updated_value);
    last_max = updated_value;
  }
  EXPECT_NEARLY_EQUAL(kTargetPlayoutDelay / 2 + kIncreasedRoundTripDelay / 2,
                      sender()->GetMaxInFlightMediaDuration(), kEpsilon);
}

// Tests that the Sender rejects frames if too large a span of FrameIds would be
// in-flight at once.
TEST_F(SenderTest, RejectsEnqueuingBeforeProtocolDesignLimit) {
  // For this test, use 1000 FPS. This makes the frames all one millisecond
  // apart to avoid triggering the media-duration rejection logic.
  constexpr int kFramesPerSecond = 1000;
  constexpr milliseconds kSmallFrameDuration{1};

  // Send the absolute design-limit maximum number of frames.
  int frame_count = 0;
  for (; frame_count < kMaxUnackedFrames; ++frame_count) {
    EncodedFrameWithBuffer frame;
    PopulateFrameWithDefaults(sender()->GetNextFrameId(), FakeClock::now(), 0,
                              13 /* bytes */, &frame);
    OverrideRtpTimestamp(frame_count, &frame, kFramesPerSecond);
    ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frame));
    SimulateExecution(kSmallFrameDuration);
  }

  // Now, attempting to enqueue just one more frame should fail.
  EncodedFrameWithBuffer one_frame_too_much;
  PopulateFrameWithDefaults(sender()->GetNextFrameId(), FakeClock::now(), 0,
                            13 /* bytes */, &one_frame_too_much);
  OverrideRtpTimestamp(frame_count++, &one_frame_too_much, kFramesPerSecond);
  EXPECT_EQ(Sender::REACHED_ID_SPAN_LIMIT,
            sender()->EnqueueFrame(one_frame_too_much));
  SimulateExecution(kSmallFrameDuration);

  // Now, simulate the Receiver ACKing the first frame, and enqueuing should
  // then succeed again.
  receiver()->SetCheckpointFrame(FrameId::first());
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.
  EXPECT_EQ(Sender::OK, sender()->EnqueueFrame(one_frame_too_much));
  SimulateExecution(kSmallFrameDuration);

  // Finally, attempting to enqueue another frame should fail again.
  EncodedFrameWithBuffer another_frame_too_much;
  PopulateFrameWithDefaults(sender()->GetNextFrameId(), FakeClock::now(), 0,
                            13 /* bytes */, &another_frame_too_much);
  OverrideRtpTimestamp(frame_count++, &another_frame_too_much,
                       kFramesPerSecond);
  EXPECT_EQ(Sender::REACHED_ID_SPAN_LIMIT,
            sender()->EnqueueFrame(another_frame_too_much));
  SimulateExecution(kSmallFrameDuration);
}

TEST_F(SenderTest, CanCancelAllInFlightFrames) {
  NiceMock<MockObserver> observer;
  sender()->SetObserver(&observer);

  // Send the absolute design-limit maximum number of frames.
  for (int i = 0; i < kMaxUnackedFrames; ++i) {
    EncodedFrameWithBuffer frame;
    PopulateFrameWithDefaults(sender()->GetNextFrameId(), FakeClock::now(), 0,
                              13 /* bytes */, &frame);
    OverrideRtpTimestamp(i, &frame, 1000 /* fps */);
    ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frame));
    SimulateExecution(kFrameDuration);
  }

  EXPECT_CALL(observer, OnFrameCanceled(_)).Times(kMaxUnackedFrames);
  sender()->CancelInFlightData();
}

// Tests that the Sender rejects frames if too-long a media duration is
// in-flight. This is the Sender's primary flow control mechanism.
TEST_F(SenderTest, RejectsEnqueuingIfTooLongMediaDurationIsInFlight) {
  // For this test, use 20 FPS. This makes all frames 50 ms apart, which should
  // make it easy to trigger the media-duration rejection logic.
  constexpr int kFramesPerSecond = 20;
  constexpr milliseconds kLargeFrameDuration{50};

  // Enqueue frames until one is rejected because the in-flight duration would
  // be too high.
  EncodedFrameWithBuffer frame;
  int frame_count = 0;
  for (; frame_count < kMaxUnackedFrames; ++frame_count) {
    PopulateFrameWithDefaults(sender()->GetNextFrameId(), FakeClock::now(), 0,
                              13 /* bytes */, &frame);
    OverrideRtpTimestamp(frame_count, &frame, kFramesPerSecond);
    const auto result = sender()->EnqueueFrame(frame);
    SimulateExecution(kLargeFrameDuration);
    if (result == Sender::MAX_DURATION_IN_FLIGHT) {
      break;
    }
    ASSERT_EQ(Sender::OK, result);
  }

  // Now, simulate the Receiver ACKing the first frame, and enqueuing should
  // then succeed again.
  receiver()->SetCheckpointFrame(FrameId::first());
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.
  EXPECT_EQ(Sender::OK, sender()->EnqueueFrame(frame));
  SimulateExecution(kLargeFrameDuration);

  // However, attempting to enqueue another frame should fail again.
  EncodedFrameWithBuffer one_frame_too_much;
  PopulateFrameWithDefaults(sender()->GetNextFrameId(), FakeClock::now(), 0,
                            13 /* bytes */, &one_frame_too_much);
  OverrideRtpTimestamp(++frame_count, &one_frame_too_much, kFramesPerSecond);
  EXPECT_EQ(Sender::MAX_DURATION_IN_FLIGHT,
            sender()->EnqueueFrame(one_frame_too_much));
  SimulateExecution(kLargeFrameDuration);
}

// Tests that the Sender propagates the Receiver's picture loss indicator to the
// Observer::OnPictureLost(), and via calls to NeedsKeyFrame(); but only when
// producing a key frame is absolutely necessary.
TEST_F(SenderTest, ManagesReceiverPictureLossWorkflow) {
  NiceMock<MockObserver> observer;
  sender()->SetObserver(&observer);

  // Send three frames...
  EncodedFrameWithBuffer frames[6];
  for (int i = 0; i < 3; ++i) {
    if (i == 0) {
      EXPECT_TRUE(sender()->NeedsKeyFrame());
    } else {
      EXPECT_FALSE(sender()->NeedsKeyFrame());
    }
    PopulateFrameWithDefaults(FrameId::first() + i,
                              FakeClock::now() - kCaptureDelay, 0,
                              24 /* bytes */, &frames[i]);
    ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frames[i]));
    SimulateExecution(kFrameDuration);
  }
  SimulateExecution(kTargetPlayoutDelay);

  // Simulate the Receiver ACK'ing the first three frames.
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first())).Times(1);
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first() + 1)).Times(1);
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first() + 2)).Times(1);
  EXPECT_CALL(observer, OnPictureLost()).Times(0);
  receiver()->SetCheckpointFrame(frames[2].frame_id);
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.
  Mock::VerifyAndClearExpectations(&observer);

  // Simulate something going wrong in the Receiver, and have it report picture
  // loss to the Sender. The Sender should then propagate this to its Observer
  // and return true when NeedsKeyFrame() is called.
  EXPECT_CALL(observer, OnFrameCanceled(_)).Times(0);
  EXPECT_CALL(observer, OnPictureLost()).Times(1);
  EXPECT_FALSE(sender()->NeedsKeyFrame());
  receiver()->SetPictureLossIndicator(true);
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.
  Mock::VerifyAndClearExpectations(&observer);
  EXPECT_TRUE(sender()->NeedsKeyFrame());

  // Send a non-key frame, and expect NeedsKeyFrame() still returns true. The
  // Observer is not re-notified. This accounts for the case where a client's
  // media encoder had frames in its processing pipeline before NeedsKeyFrame()
  // began returning true.
  EXPECT_CALL(observer, OnFrameCanceled(_)).Times(0);
  EXPECT_CALL(observer, OnPictureLost()).Times(0);
  EncodedFrameWithBuffer& nonkey_frame = frames[3];
  PopulateFrameWithDefaults(FrameId::first() + 3,
                            FakeClock::now() - kCaptureDelay, 0, 24 /* bytes */,
                            &nonkey_frame);
  ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(nonkey_frame));
  SimulateExecution(kFrameDuration);
  Mock::VerifyAndClearExpectations(&observer);
  EXPECT_TRUE(sender()->NeedsKeyFrame());

  // Now send a key frame, and expect NeedsKeyFrame() returns false. Note that
  // the Receiver hasn't cleared the PLI condition, but the Sender knows more
  // key frames won't be needed.
  EXPECT_CALL(observer, OnFrameCanceled(_)).Times(0);
  EXPECT_CALL(observer, OnPictureLost()).Times(0);
  EncodedFrameWithBuffer& recovery_frame = frames[4];
  PopulateFrameWithDefaults(FrameId::first() + 4,
                            FakeClock::now() - kCaptureDelay, 0, 24 /* bytes */,
                            &recovery_frame);
  recovery_frame.dependency = EncodedFrame::Dependency::kKeyFrame;
  recovery_frame.referenced_frame_id = recovery_frame.frame_id;
  ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(recovery_frame));
  SimulateExecution(kFrameDuration);
  Mock::VerifyAndClearExpectations(&observer);
  EXPECT_FALSE(sender()->NeedsKeyFrame());

  // Let's say the Receiver hasn't received the key frame yet, and it reports
  // its picture loss again to the Sender. Observer::OnPictureLost() should not
  // be called, and NeedsKeyFrame() should NOT return true, because the Sender
  // knows the Receiver hasn't acknowledged the key frame (just sent) yet.
  EXPECT_CALL(observer, OnFrameCanceled(nonkey_frame.frame_id)).Times(1);
  EXPECT_CALL(observer, OnPictureLost()).Times(0);
  receiver()->SetCheckpointFrame(nonkey_frame.frame_id);
  receiver()->SetPictureLossIndicator(true);
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.
  Mock::VerifyAndClearExpectations(&observer);
  EXPECT_FALSE(sender()->NeedsKeyFrame());

  // Now, simulate the Receiver getting the key frame, but NOT recovering. This
  // should cause Observer::OnPictureLost() to be called, and cause
  // NeedsKeyFrame() to return true again.
  EXPECT_CALL(observer, OnFrameCanceled(recovery_frame.frame_id)).Times(1);
  EXPECT_CALL(observer, OnPictureLost()).Times(1);
  receiver()->SetCheckpointFrame(recovery_frame.frame_id);
  receiver()->SetPictureLossIndicator(true);
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.
  Mock::VerifyAndClearExpectations(&observer);
  EXPECT_TRUE(sender()->NeedsKeyFrame());

  // Send another key frame, and expect NeedsKeyFrame() returns false.
  EXPECT_CALL(observer, OnFrameCanceled(_)).Times(0);
  EXPECT_CALL(observer, OnPictureLost()).Times(0);
  EncodedFrameWithBuffer& another_recovery_frame = frames[5];
  PopulateFrameWithDefaults(FrameId::first() + 5,
                            FakeClock::now() - kCaptureDelay, 0, 24 /* bytes */,
                            &another_recovery_frame);
  another_recovery_frame.dependency = EncodedFrame::Dependency::kKeyFrame;
  another_recovery_frame.referenced_frame_id = another_recovery_frame.frame_id;
  ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(another_recovery_frame));
  SimulateExecution(kFrameDuration);
  Mock::VerifyAndClearExpectations(&observer);
  EXPECT_FALSE(sender()->NeedsKeyFrame());

  // Now, simulate the Receiver recovering. It will report this to the Sender,
  // and NeedsKeyFrame() will still return false.
  EXPECT_CALL(observer, OnFrameCanceled(another_recovery_frame.frame_id))
      .Times(1);
  EXPECT_CALL(observer, OnPictureLost()).Times(0);
  receiver()->SetCheckpointFrame(another_recovery_frame.frame_id);
  receiver()->SetPictureLossIndicator(false);
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.
  Mock::VerifyAndClearExpectations(&observer);
  EXPECT_FALSE(sender()->NeedsKeyFrame());

  ExpectFramesReceivedCorrectly(frames, receiver()->TakeCompleteFrames());
}

// Tests that the Receiver should get a Sender Report just before the first RTP
// packet, and at regular intervals thereafter. The Sender Report contains the
// lip-sync information necessary for play-out timing.
TEST_F(SenderTest, ProvidesSenderReports) {
  std::vector<SenderReportParser::SenderReportWithId> sender_reports;
  Sequence packet_sequence;
  EXPECT_CALL(*receiver(), OnSenderReport(_))
      .InSequence(packet_sequence)
      .WillOnce(
          Invoke([&](const SenderReportParser::SenderReportWithId& report) {
            sender_reports.push_back(report);
          }))
      .RetiresOnSaturation();
  EXPECT_CALL(*receiver(), OnRtpPacket(_)).Times(1).InSequence(packet_sequence);
  EXPECT_CALL(*receiver(), OnSenderReport(_))
      .Times(3)
      .InSequence(packet_sequence)
      .WillRepeatedly(
          Invoke([&](const SenderReportParser::SenderReportWithId& report) {
            sender_reports.push_back(report);
          }));

  EncodedFrameWithBuffer frame;
  constexpr int kFrameDataSize = 250;
  PopulateFrameWithDefaults(FrameId::first(), FakeClock::now(), 0,
                            kFrameDataSize, &frame);
  ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frame));
  SimulateExecution();  // Should send one Sender Report + one RTP packet.
  EXPECT_EQ(size_t{1}, sender_reports.size());

  // Have the Receiver ACK the frame to prevent retransmitting the RTP packet.
  receiver()->SetCheckpointFrame(FrameId::first());
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.

  // Advance through three more reporting intervals. One Sender Report should be
  // sent each interval, making a total of 4 reports sent.
  constexpr auto kThreeReportIntervals = 3 * kRtcpReportInterval;
  SimulateExecution(kThreeReportIntervals);  // Three more Sender Reports.
  ASSERT_EQ(size_t{4}, sender_reports.size());

  // The first report should contain the same timestamps as the frame because
  // the Clock did not advance. Also, its packet count and octet count fields
  // should be zero since the report was sent before the RTP packet.
  EXPECT_EQ(frame.reference_time, sender_reports.front().reference_time);
  EXPECT_EQ(frame.rtp_timestamp, sender_reports.front().rtp_timestamp);
  EXPECT_EQ(uint32_t{0}, sender_reports.front().send_packet_count);
  EXPECT_EQ(uint32_t{0}, sender_reports.front().send_octet_count);

  // The last report should contain the timestamps extrapolated into the future
  // because the Clock did move forward. Also, the packet count and octet fields
  // should now be non-zero because the report was sent after the RTP packet.
  EXPECT_EQ(frame.reference_time + kThreeReportIntervals,
            sender_reports.back().reference_time);
  EXPECT_EQ(frame.rtp_timestamp +
                RtpTimeDelta::FromDuration(kThreeReportIntervals, kRtpTimebase),
            sender_reports.back().rtp_timestamp);
  EXPECT_EQ(uint32_t{1}, sender_reports.back().send_packet_count);
  EXPECT_EQ(uint32_t{kFrameDataSize}, sender_reports.back().send_octet_count);
}

// Tests that the Sender provides Kickstart packets whenever the Receiver may
// not know about new frames.
TEST_F(SenderTest, ProvidesKickstartPacketsIfReceiverDoesNotACK) {
  // Have the Receiver move the checkpoint forward only for the first frame, and
  // none of the later frames. This will force the Sender to eventually send a
  // Kickstart packet.
  ON_CALL(*receiver(), OnFrameComplete(_))
      .WillByDefault(Invoke([&](FrameId frame_id) {
        if (frame_id == FrameId::first()) {
          receiver()->SetCheckpointFrame(FrameId::first());
          receiver()->TransmitRtcpFeedbackPacket();
        }
      }));

  // Send three frames, paced to the media.
  EncodedFrameWithBuffer frames[3];
  for (int i = 0; i < 3; ++i) {
    PopulateFrameWithDefaults(FrameId::first() + i,
                              FakeClock::now() - kCaptureDelay, i,
                              48 /* bytes */, &frames[i]);
    ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frames[i]));
    SimulateExecution(kFrameDuration);
  }

  // Now, do nothing for a while. Because the Receiver isn't moving the
  // checkpoint forward, the Sender will have sent all the RTP packets at least
  // once, and then will start sending just Kickstart packets.
  SimulateExecution(kTargetPlayoutDelay);

  // Keep doing nothing for a while, and confirm the Sender is just sending the
  // same Kickstart packet over and over. The Kickstart packet is supposed to be
  // the last packet of the latest frame.
  std::set<std::pair<FrameId, FramePacketId>> unique_received_packet_ids;
  EXPECT_CALL(*receiver(), OnRtpPacket(_))
      .WillRepeatedly(
          Invoke([&](const RtpPacketParser::ParseResult& parsed_packet) {
            unique_received_packet_ids.emplace(parsed_packet.frame_id,
                                               parsed_packet.packet_id);
          }));
  SimulateExecution(kTargetPlayoutDelay);
  Mock::VerifyAndClearExpectations(receiver());
  EXPECT_EQ(size_t{1}, unique_received_packet_ids.size());
  EXPECT_EQ(frames[2].frame_id, unique_received_packet_ids.begin()->first);

  // Now, simulate the Receiver ACKing all the frames.
  receiver()->SetCheckpointFrame(frames[2].frame_id);
  receiver()->TransmitRtcpFeedbackPacket();
  SimulateExecution();  // RTCP transmitted to Sender.

  // With all the frames sent, the Sender should not be transmitting anything.
  EXPECT_CALL(*receiver(), OnRtpPacket(_)).Times(0);
  SimulateExecution(10 * kTargetPlayoutDelay);

  ExpectFramesReceivedCorrectly(frames, receiver()->TakeCompleteFrames());
}

// Tests that the Sender only retransmits packets specifically NACK'ed by the
// Receiver.
TEST_F(SenderTest, ResendsIndividuallyNackedPackets) {
  // Populate the frame data in each frame with enough bytes to force at least
  // three RTP packets per frame.
  constexpr int kFrameDataSize = 3 * kMaxRtpPacketSizeForIpv6UdpOnEthernet;

  // Use a 1ms network delay in each direction to make the sequence of events
  // clearer in this test.
  constexpr milliseconds kOneWayNetworkDelay{1};
  SetSenderToReceiverNetworkDelay(kOneWayNetworkDelay);
  SetReceiverToSenderNetworkDelay(kOneWayNetworkDelay);

  // Simulate that three specific packets will be dropped by the network, one
  // from each frame (about to be sent).
  const std::vector<PacketNack> dropped_packets{
      {FrameId::first(), FramePacketId{2}},
      {FrameId::first() + 1, FramePacketId{1}},
      {FrameId::first() + 2, FramePacketId{0}},
  };
  receiver()->SetIgnoreList(dropped_packets);

  // Send three frames, paced to the media. The Receiver won't completely
  // receive any of these frames due to dropped packets.
  EXPECT_CALL(*receiver(), OnFrameComplete(_)).Times(0);
  EncodedFrameWithBuffer frames[3];
  for (int i = 0; i < 3; ++i) {
    PopulateFrameWithDefaults(FrameId::first() + i,
                              FakeClock::now() - kCaptureDelay, i,
                              kFrameDataSize, &frames[i]);
    ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frames[i]));
    SimulateExecution(kFrameDuration);
  }
  SimulateExecution(kTargetPlayoutDelay);
  Mock::VerifyAndClearExpectations(receiver());
  EXPECT_EQ(3, sender()->GetInFlightFrameCount());

  // The Receiver NACKs the three dropped packets...
  receiver()->SetNacksAndAcks(dropped_packets, {});
  receiver()->TransmitRtcpFeedbackPacket();

  // In the meantime, the network recovers (i.e., no more dropped packets)...
  receiver()->SetIgnoreList({});

  // The NACKs reach the Sender, and it acts on them by retransmitting.
  SimulateExecution(kOneWayNetworkDelay);

  // As each retransmitted packet arrives at the Receiver, advance the
  // checkpoint forward to notify the Sender of frames that are now completely
  // received. Also, confirm that only the three specifically-NACK'ed packets
  // were retransmitted.
  EXPECT_CALL(*receiver(), OnFrameComplete(_))
      .Times(3)
      .WillRepeatedly(InvokeWithoutArgs([&] {
        if (receiver()->AutoAdvanceCheckpoint()) {
          receiver()->TransmitRtcpFeedbackPacket();
        }
      }));
  EXPECT_CALL(*receiver(), OnRtpPacket(_))
      .Times(3)
      .WillRepeatedly(Invoke([&](const RtpPacketParser::ParseResult& packet) {
        EXPECT_TRUE(Contains(dropped_packets,
                             PacketNack{packet.frame_id, packet.packet_id}));
      }));
  SimulateExecution(kOneWayNetworkDelay);
  Mock::VerifyAndClearExpectations(receiver());

  // The Receiver checkpoint feedback(s) travel back to the Sender, and there
  // should no longer be any frames in-flight.
  SimulateExecution(kOneWayNetworkDelay);
  EXPECT_EQ(0, sender()->GetInFlightFrameCount());

  // The Sender should not be transmitting anything from now on since all frames
  // are known to have been completely received.
  EXPECT_CALL(*receiver(), OnRtpPacket(_)).Times(0);
  SimulateExecution(10 * kTargetPlayoutDelay);

  ExpectFramesReceivedCorrectly(frames, receiver()->TakeCompleteFrames());
}

// Tests that the Sender retransmits an entire frame if the Receiver requests it
// (i.e., a full frame NACK), but does not retransmit any packets for frames
// (before or after) that have been acknowledged.
TEST_F(SenderTest, ResendsMissingFrames) {
  // Populate the frame data in each frame with enough bytes to force at least
  // three RTP packets per frame.
  constexpr int kFrameDataSize = 3 * kMaxRtpPacketSizeForIpv6UdpOnEthernet;

  // Use a 1ms network delay in each direction to make the sequence of events
  // clearer in this test.
  constexpr milliseconds kOneWayNetworkDelay{1};
  SetSenderToReceiverNetworkDelay(kOneWayNetworkDelay);
  SetReceiverToSenderNetworkDelay(kOneWayNetworkDelay);

  // Simulate that all of the packets for the second frame will be dropped by
  // the network, but only the packets for that frame.
  const std::vector<PacketNack> dropped_packets{
      {FrameId::first() + 1, kAllPacketsLost},
  };
  receiver()->SetIgnoreList(dropped_packets);

  NiceMock<MockObserver> observer;
  sender()->SetObserver(&observer);

  // The expectations below track the story and execute simulated Receiver
  // responses. The Sender will have three frames enqueued by its client, and
  // then...
  //
  // The first frame is received and the Receiver ACKs it by moving the
  // checkpoint forward.
  Sequence completion_sequence;
  EXPECT_CALL(*receiver(), OnFrameComplete(FrameId::first()))
      .InSequence(completion_sequence)
      .WillOnce(InvokeWithoutArgs([&] {
        receiver()->SetCheckpointFrame(FrameId::first());
        receiver()->TransmitRtcpFeedbackPacket();
      }));
  // Since all of the packets for the second frame are being dropped, the third
  // frame will finish next. The Receiver responds by NACKing the second frame
  // and ACKing the third frame. The checkpoint does not move forward because
  // the second frame has not been received yet.
  //
  // NETWORK CHANGE: After the third frame is received, stop dropping packets.
  EXPECT_CALL(*receiver(), OnFrameComplete(FrameId::first() + 2))
      .InSequence(completion_sequence)
      .WillOnce(InvokeWithoutArgs([&] {
        receiver()->SetNacksAndAcks(dropped_packets,
                                    std::vector<FrameId>{FrameId::first() + 2});
        receiver()->TransmitRtcpFeedbackPacket();
        receiver()->SetIgnoreList({});
      }));
  // Finally, the Sender should respond to the whole-frame NACK by re-sending
  // all of the packets for the second frame, and so the Receiver should
  // completely receive the frame.
  EXPECT_CALL(*receiver(), OnFrameComplete(FrameId::first() + 1))
      .InSequence(completion_sequence)
      .WillOnce(InvokeWithoutArgs([&] {
        receiver()->SetCheckpointFrame(FrameId::first() + 2);
        receiver()->TransmitRtcpFeedbackPacket();
      }));

  // From the Sender's perspective, the Receiver will ACK the first frame, then
  // the third frame, then the second frame.
  Sequence cancel_sequence;
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first()))
      .Times(1)
      .InSequence(cancel_sequence);
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first() + 2))
      .Times(1)
      .InSequence(cancel_sequence);
  EXPECT_CALL(observer, OnFrameCanceled(FrameId::first() + 1))
      .Times(1)
      .InSequence(cancel_sequence);

  // With all the expectations/sequences in-place, let 'er rip!
  EncodedFrameWithBuffer frames[3];
  for (int i = 0; i < 3; ++i) {
    PopulateFrameWithDefaults(FrameId::first() + i,
                              FakeClock::now() - kCaptureDelay, i,
                              kFrameDataSize, &frames[i]);
    ASSERT_EQ(Sender::OK, sender()->EnqueueFrame(frames[i]));
    SimulateExecution(kFrameDuration);
  }
  SimulateExecution(kTargetPlayoutDelay);
  Mock::VerifyAndClearExpectations(receiver());
  EXPECT_EQ(0, sender()->GetInFlightFrameCount());

  // The Sender should not be transmitting anything from now on since all frames
  // are known to have been completely received.
  EXPECT_CALL(*receiver(), OnRtpPacket(_)).Times(0);
  SimulateExecution(10 * kTargetPlayoutDelay);

  ExpectFramesReceivedCorrectly(frames, receiver()->TakeCompleteFrames());
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
