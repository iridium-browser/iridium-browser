// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/sender_packet_router.h"

#include <chrono>

#include "cast/streaming/constants.h"
#include "cast/streaming/mock_environment.h"
#include "cast/streaming/testing/simple_socket_subscriber.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/base/ip_address.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "util/big_endian.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"

using testing::_;
using testing::Invoke;
using testing::Mock;
using testing::Return;

namespace openscreen {
namespace cast {
namespace {

const IPEndpoint kRemoteEndpoint{
    // Use a random IPv6 address in the range reserved for "documentation
    // purposes."
    IPAddress::Parse("2001:db8:0d93:69c2:fd1a:49a6:a7c0:e8a6").value(), 25476};

const IPEndpoint kUnexpectedEndpoint{
    IPAddress::Parse("2001:db8:0d93:69c2:fd1a:49a6:a7c0:e8a7").value(), 25476};

// Limited burst parameters to simplify unit testing.
constexpr int kMaxPacketsPerBurst = 3;
constexpr auto kBurstInterval = milliseconds(10);

constexpr Ssrc kAudioReceiverSsrc = 2;
constexpr Ssrc kVideoReceiverSsrc = 32;

const uint8_t kGarbagePacket[] = {
    0x42, 0x61, 0x16, 0x17, 0x26, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6d, 0x69,
    0x6e, 0x67, 0x2f, 0x63, 0x61, 0x73, 0x74, 0x2f, 0x63, 0x6f, 0x6d, 0x70,
    0x6f, 0x75, 0x6e, 0x64, 0x5f, 0x72, 0x74, 0x63, 0x70, 0x5f};

// clang-format off
const uint8_t kValidAudioRtcpPacket[] = {
    0b10000000,  // Version=2, Padding=no, ReportCount=0.
    201,  // RTCP Packet type byte.
    0x00, 0x01,  // Length of remainder of packet, in 32-bit words.
    0x00, 0x00, 0x00, 0x02,  // Receiver SSRC.
};

const uint8_t kValidAudioRtpPacket[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xbe, 0xef,  // Sequence number.
    9, 8, 7, 6,  // RTP timestamp.
    0, 0, 0, 2,  // SSRC.
    0b10000000,  // Is key frame, no extensions.
    5,  // Frame ID.
    0xa, 0xb,  // Packet ID.
    0xa, 0xc,  // Max packet ID.
    0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8,  // Payload.
};
// clang-format on

// Returns a copy of an |original| RTCP packet, but with its send-to SSRC
// modified to the given |alternate_ssrc|.
std::vector<uint8_t> MakeRtcpPacketWithAlternateReceiverSsrc(
    absl::Span<const uint8_t> original,
    Ssrc alternate_ssrc) {
  constexpr int kOffsetToSsrcField = 4;
  std::vector<uint8_t> out(original.begin(), original.end());
  OSP_CHECK_GE(out.size(), kOffsetToSsrcField + sizeof(uint32_t));
  WriteBigEndian(uint32_t{alternate_ssrc}, out.data() + kOffsetToSsrcField);
  return out;
}

// Serializes the |flag| and |send_time| into the front of |buffer| so the tests
// can make unique packets and confirm their identities after passing through
// various components.
absl::Span<uint8_t> MakeFakePacketWithFlag(char flag,
                                           Clock::time_point send_time,
                                           absl::Span<uint8_t> buffer) {
  const Clock::duration::rep ticks = send_time.time_since_epoch().count();
  const auto packet_size = sizeof(ticks) + sizeof(flag);
  buffer = buffer.subspan(0, packet_size);
  OSP_CHECK_EQ(buffer.size(), packet_size);
  WriteBigEndian(ticks, buffer.data());
  buffer[sizeof(ticks)] = flag;
  return buffer;
}

// Same as MakeFakePacketWithFlag(), but for tests that don't use the flag.
absl::Span<uint8_t> MakeFakePacket(Clock::time_point send_time,
                                   absl::Span<uint8_t> buffer) {
  return MakeFakePacketWithFlag('?', send_time, buffer);
}

// Returns the flag that was placed in the given |fake_packet|, or '?' if
// unknown.
char ParseFlag(absl::Span<const uint8_t> fake_packet) {
  constexpr auto kFlagOffset = sizeof(Clock::duration::rep);
  if (fake_packet.size() == (kFlagOffset + sizeof(char))) {
    return static_cast<char>(fake_packet[kFlagOffset]);
  }
  return '?';
}

// Deserializes and returns the timestamp that was placed in the given |packet|
// by MakeFakePacketWithFlag().
Clock::time_point ParseTimestamp(absl::Span<const uint8_t> fake_packet) {
  Clock::duration::rep ticks = 0;
  if (fake_packet.size() >= sizeof(ticks)) {
    ticks = ReadBigEndian<Clock::duration::rep>(fake_packet.data());
  }
  return Clock::time_point() + Clock::duration(ticks);
}

// Returns an empty version of |buffer|.
absl::Span<uint8_t> ToEmptyPacketBuffer(Clock::time_point send_time,
                                        absl::Span<uint8_t> buffer) {
  return buffer.subspan(0, 0);
}

class MockSender : public SenderPacketRouter::Sender {
 public:
  MockSender() = default;
  ~MockSender() override = default;

  MOCK_METHOD(void,
              OnReceivedRtcpPacket,
              (Clock::time_point arrival_time,
               absl::Span<const uint8_t> packet),
              (override));
  MOCK_METHOD(absl::Span<uint8_t>,
              GetRtcpPacketForImmediateSend,
              (Clock::time_point send_time, absl::Span<uint8_t> buffer),
              (override));
  MOCK_METHOD(absl::Span<uint8_t>,
              GetRtpPacketForImmediateSend,
              (Clock::time_point send_time, absl::Span<uint8_t> buffer),
              (override));
  MOCK_METHOD(Clock::time_point, GetRtpResumeTime, (), (override));
};

class SenderPacketRouterTest : public testing::Test {
 public:
  SenderPacketRouterTest()
      : clock_(Clock::now()),
        task_runner_(&clock_),
        env_(&FakeClock::now, &task_runner_),
        router_(&env_, kMaxPacketsPerBurst, kBurstInterval) {
    env_.SetSocketSubscriber(&socket_subscriber_);
  }

  ~SenderPacketRouterTest() override = default;

  MockEnvironment* env() { return &env_; }
  SenderPacketRouter* router() { return &router_; }
  MockSender* audio_sender() { return &audio_sender_; }
  MockSender* video_sender() { return &video_sender_; }

  void SimulatePacketArrivedNow(const IPEndpoint& source,
                                absl::Span<const uint8_t> packet) {
    static_cast<Environment::PacketConsumer*>(&router_)->OnReceivedPacket(
        source, env_.now(), std::vector<uint8_t>(packet.begin(), packet.end()));
  }

  void AdvanceClockAndRunTasks(Clock::duration delta) { clock_.Advance(delta); }
  void RunTasksUntilIdle() { task_runner_.RunTasksUntilIdle(); }

 private:
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  testing::NiceMock<MockEnvironment> env_;
  SenderPacketRouter router_;
  testing::NiceMock<MockSender> audio_sender_;
  testing::NiceMock<MockSender> video_sender_;
  SimpleSubscriber socket_subscriber_;
};

// Tests that the SenderPacketRouter is correctly configured from the specific
// burst parameters that were passed to its constructor. This confirms internal
// calculations based on these parameters.
TEST_F(SenderPacketRouterTest, IsConfiguredFromBurstParameters) {
  EXPECT_EQ(env()->GetMaxPacketSize(), router()->max_packet_size());

  // The following lower-bound/upper-bound values were hand-calculated based on
  // the arguments that were passed to the SenderPacketRouter constructor, and
  // assuming a packet size anywhere from 256 bytes to one megabyte.
  //
  // The exact value for max_burst_bitrate() is not known here because
  // Environment::GetMaxPacketSize() depends on the platform and network medium.
  // To test for an exact value would require duplicating the math in
  // SenderPacketRouter::ComputeMaxBurstBitrate() here (and then *what* would we
  // be testing?).
  EXPECT_LE(614400, router()->max_burst_bitrate());
  EXPECT_GE(2147483647, router()->max_burst_bitrate());
}

TEST_F(SenderPacketRouterTest, IgnoresPacketsFromUnexpectedSources) {
  env()->set_remote_endpoint(kRemoteEndpoint);
  router()->OnSenderCreated(kAudioReceiverSsrc, audio_sender());
  EXPECT_CALL(*audio_sender(), OnReceivedRtcpPacket(_, _)).Times(0);
  SimulatePacketArrivedNow(kUnexpectedEndpoint,
                           absl::Span<const uint8_t>(kValidAudioRtcpPacket));
  router()->OnSenderDestroyed(kAudioReceiverSsrc);
}

TEST_F(SenderPacketRouterTest, IgnoresInboundPacketsContainingGarbage) {
  env()->set_remote_endpoint(kRemoteEndpoint);
  router()->OnSenderCreated(kAudioReceiverSsrc, audio_sender());
  EXPECT_CALL(*audio_sender(), OnReceivedRtcpPacket(_, _)).Times(0);
  SimulatePacketArrivedNow(kUnexpectedEndpoint,
                           absl::Span<const uint8_t>(kGarbagePacket));
  SimulatePacketArrivedNow(kRemoteEndpoint,
                           absl::Span<const uint8_t>(kGarbagePacket));
  router()->OnSenderDestroyed(kAudioReceiverSsrc);
}

// Note: RTP packets should be ignored since it wouldn't make sense for a
// Receiver to stream media to a Sender.
TEST_F(SenderPacketRouterTest, IgnoresInboundRtpPackets) {
  env()->set_remote_endpoint(kRemoteEndpoint);
  router()->OnSenderCreated(kAudioReceiverSsrc, audio_sender());
  EXPECT_CALL(*audio_sender(), OnReceivedRtcpPacket(_, _)).Times(0);
  SimulatePacketArrivedNow(kUnexpectedEndpoint,
                           absl::Span<const uint8_t>(kValidAudioRtpPacket));
  SimulatePacketArrivedNow(kRemoteEndpoint,
                           absl::Span<const uint8_t>(kValidAudioRtpPacket));
  router()->OnSenderDestroyed(kAudioReceiverSsrc);
}

TEST_F(SenderPacketRouterTest, IgnoresInboundRtcpPacketsFromUnknownReceivers) {
  env()->set_remote_endpoint(kRemoteEndpoint);
  router()->OnSenderCreated(kAudioReceiverSsrc, audio_sender());
  const std::vector<uint8_t> rtcp_packet_not_for_me =
      MakeRtcpPacketWithAlternateReceiverSsrc(kValidAudioRtcpPacket,
                                              kAudioReceiverSsrc + 1);
  EXPECT_CALL(*audio_sender(), OnReceivedRtcpPacket(_, _)).Times(0);
  SimulatePacketArrivedNow(kUnexpectedEndpoint,
                           absl::Span<const uint8_t>(rtcp_packet_not_for_me));
  SimulatePacketArrivedNow(kRemoteEndpoint,
                           absl::Span<const uint8_t>(rtcp_packet_not_for_me));
  router()->OnSenderDestroyed(kAudioReceiverSsrc);
}

// Tests that the SenderPacketRouter forwards packets from Receivers to the
// appropriate Sender.
TEST_F(SenderPacketRouterTest, RoutesRTCPPacketsFromReceivers) {
  EXPECT_CALL(*env(), SendPacket(_)).Times(0);

  const absl::Span<const uint8_t> audio_rtcp_packet(kValidAudioRtcpPacket);
  std::vector<uint8_t> video_rtcp_packet =
      MakeRtcpPacketWithAlternateReceiverSsrc(audio_rtcp_packet,
                                              kVideoReceiverSsrc);

  env()->set_remote_endpoint(kRemoteEndpoint);
  router()->OnSenderCreated(kAudioReceiverSsrc, audio_sender());

  // It should route a valid audio RTCP packet to the audio Sender, and ignore a
  // valid video RTCP packet (since the video Sender is not yet known to the
  // SenderPacketRouter).
  {
    Clock::time_point arrival_time{};
    std::vector<uint8_t> received_packet;
    EXPECT_CALL(*audio_sender(), OnReceivedRtcpPacket(_, _))
        .WillOnce(Invoke(
            [&](Clock::time_point when, absl::Span<const uint8_t> packet) {
              arrival_time = when;
              received_packet.assign(packet.begin(), packet.end());
            }));
    EXPECT_CALL(*video_sender(), OnReceivedRtcpPacket(_, _)).Times(0);

    const Clock::time_point expected_arrival_time = env()->now();
    SimulatePacketArrivedNow(kRemoteEndpoint, audio_rtcp_packet);
    SimulatePacketArrivedNow(kRemoteEndpoint, video_rtcp_packet);

    Mock::VerifyAndClear(audio_sender());
    EXPECT_EQ(expected_arrival_time, arrival_time);
    EXPECT_EQ(audio_rtcp_packet, received_packet);

    Mock::VerifyAndClear(video_sender());
  }

  AdvanceClockAndRunTasks(seconds(1));

  // Register the video Sender with the router. Now, confirm audio RTCP packets
  // still go to the audio Sender and video RTCP packets go to the video Sender.
  router()->OnSenderCreated(kVideoReceiverSsrc, video_sender());
  {
    Clock::time_point audio_arrival_time{}, video_arrival_time{};
    std::vector<uint8_t> received_audio_packet, received_video_packet;
    EXPECT_CALL(*audio_sender(), OnReceivedRtcpPacket(_, _))
        .WillOnce(Invoke(
            [&](Clock::time_point when, absl::Span<const uint8_t> packet) {
              audio_arrival_time = when;
              received_audio_packet.assign(packet.begin(), packet.end());
            }));
    EXPECT_CALL(*video_sender(), OnReceivedRtcpPacket(_, _))
        .WillOnce(Invoke(
            [&](Clock::time_point when, absl::Span<const uint8_t> packet) {
              video_arrival_time = when;
              received_video_packet.assign(packet.begin(), packet.end());
            }));

    const Clock::time_point expected_audio_arrival_time = env()->now();
    SimulatePacketArrivedNow(kRemoteEndpoint, audio_rtcp_packet);

    AdvanceClockAndRunTasks(milliseconds(11));

    const Clock::time_point expected_video_arrival_time = env()->now();
    SimulatePacketArrivedNow(kRemoteEndpoint, video_rtcp_packet);

    Mock::VerifyAndClear(audio_sender());
    EXPECT_EQ(expected_audio_arrival_time, audio_arrival_time);
    EXPECT_EQ(audio_rtcp_packet, received_audio_packet);

    Mock::VerifyAndClear(video_sender());
    EXPECT_EQ(expected_video_arrival_time, video_arrival_time);
    EXPECT_EQ(video_rtcp_packet, received_video_packet);
  }

  router()->OnSenderDestroyed(kAudioReceiverSsrc);
  router()->OnSenderDestroyed(kVideoReceiverSsrc);
}

// Tests that the SenderPacketRouter schedules periodic RTCP packet sends,
// starting once the Sender requests the first RTCP send.
TEST_F(SenderPacketRouterTest, SchedulesPeriodicTransmissionOfRTCPPackets) {
  env()->set_remote_endpoint(kRemoteEndpoint);
  router()->OnSenderCreated(kAudioReceiverSsrc, audio_sender());

  constexpr int kNumIterations = 5;

  EXPECT_CALL(*audio_sender(), OnReceivedRtcpPacket(_, _)).Times(0);
  EXPECT_CALL(*audio_sender(), GetRtcpPacketForImmediateSend(_, _))
      .Times(kNumIterations)
      .WillRepeatedly(Invoke(&MakeFakePacket));
  EXPECT_CALL(*audio_sender(), GetRtpPacketForImmediateSend(_, _)).Times(0);
  ON_CALL(*audio_sender(), GetRtpResumeTime())
      .WillByDefault(Return(SenderPacketRouter::kNever));

  // Capture every packet sent for analysis at the end of this test.
  std::vector<std::vector<uint8_t>> packets_sent;
  EXPECT_CALL(*env(), SendPacket(_))
      .WillRepeatedly(Invoke([&](absl::Span<const uint8_t> packet) {
        packets_sent.emplace_back(packet.begin(), packet.end());
      }));

  const Clock::time_point first_send_time = env()->now();
  router()->RequestRtcpSend(kAudioReceiverSsrc);
  RunTasksUntilIdle();  // The first RTCP packet should be sent immediately.
  for (int i = 1; i < kNumIterations; ++i) {
    AdvanceClockAndRunTasks(kRtcpReportInterval);
  }

  // Ensure each RTCP packet was sent and in-sequence.
  Mock::VerifyAndClear(env());
  ASSERT_EQ(kNumIterations, static_cast<int>(packets_sent.size()));
  for (int i = 0; i < kNumIterations; ++i) {
    const Clock::time_point expected_send_time =
        first_send_time + i * kRtcpReportInterval;
    EXPECT_EQ(expected_send_time, ParseTimestamp(packets_sent[i]));
  }

  router()->OnSenderDestroyed(kAudioReceiverSsrc);
}

// Tests that the SenderPacketRouter schedules RTP packet bursts from a single
// Sender.
TEST_F(SenderPacketRouterTest, SchedulesAndTransmitsRTPBursts) {
  env()->set_remote_endpoint(kRemoteEndpoint);
  router()->OnSenderCreated(kVideoReceiverSsrc, video_sender());

  // Capture every packet sent for analysis at the end of this test.
  std::vector<std::vector<uint8_t>> packets_sent;
  EXPECT_CALL(*env(), SendPacket(_))
      .WillRepeatedly(Invoke([&](absl::Span<const uint8_t> packet) {
        packets_sent.emplace_back(packet.begin(), packet.end());
      }));

  // Simulate a typical video Sender RTP at-startup sending sequence: First, at
  // t=0ms, the Sender wants to send its large 10-packet key frame. This will
  // require four bursts, since only 3 packets can be sent per burst.
  //
  // While the first frame is being sent, a smaller 4-packet frame is enqueued,
  // and the Sender will want to start sending this immediately after the first
  // frame. Part of this second frame will be sent in the fourth burst, and the
  // rest in the fifth burst.
  //
  // After the fifth burst, the Sender will schedule a "kickstart packet" for
  // 25ms later. However, when the SenderPacketRouter later asks the Sender for
  // that packet, the Sender will change its mind and decide not to send
  // anything.
  //
  // At t=100ms, the next frame of video is enqueued in the Sender and it
  // requests that RTP sending resume for that. This is a small 1-packet frame.
  const Clock::time_point start_time = env()->now();
  int num_get_rtp_calls = 0;
  EXPECT_CALL(*video_sender(), GetRtpPacketForImmediateSend(_, _))
      .Times(14 + 2)
      .WillRepeatedly(
          Invoke([&](Clock::time_point send_time, absl::Span<uint8_t> buffer) {
            ++num_get_rtp_calls;

            // 14 packets are sent: The first through fourth bursts send three
            // packets each, and the fifth burst sends two.
            if (num_get_rtp_calls <= 14) {
              return MakeFakePacket(send_time, buffer);
            }

            // 2 "done signals" are then sent: One is at the end of the fifth
            // burst, one is for a "nothing to send" sixth burst.
            return ToEmptyPacketBuffer(send_time, buffer);
          }));
  const Clock::time_point kickstart_time =
      start_time + 4 * kBurstInterval + milliseconds(25);
  int num_get_resume_calls = 0;
  EXPECT_CALL(*video_sender(), GetRtpResumeTime())
      .Times(4 + 1 + 1)
      .WillRepeatedly(Invoke([&] {
        ++num_get_resume_calls;

        // After each of the first through fourth bursts, the Sender wants to
        // transmit more right away.
        if (num_get_resume_calls <= 4) {
          return env()->now();
        }

        // After the fifth burst, the Sender requests resuming for kickstart
        // later.
        if (num_get_resume_calls == 5) {
          return kickstart_time;
        }

        // After the sixth burst, the Sender pauses RTP sending indefinitely.
        return SenderPacketRouter::kNever;
      }));
  router()->RequestRtpSend(kVideoReceiverSsrc);
  // Execute first burst.
  RunTasksUntilIdle();
  // Execute second through fifth bursts.
  for (int i = 1; i <= 4; ++i) {
    AdvanceClockAndRunTasks(kBurstInterval);
  }
  // Execute the sixth burst at the kickstart time.
  AdvanceClockAndRunTasks(kickstart_time - env()->now());
  Mock::VerifyAndClear(video_sender());

  // Now, resume RTP sending for one more 1-packet frame, and then pause RTP
  // sending again.
  EXPECT_CALL(*video_sender(), GetRtpPacketForImmediateSend(_, _))
      .WillOnce(Invoke(&MakeFakePacket))        // Frame 2, only packet.
      .WillOnce(Invoke(&ToEmptyPacketBuffer));  // Done for now.
  // After the seventh burst, the Sender pauses RTP sending again.
  EXPECT_CALL(*video_sender(), GetRtpResumeTime())
      .WillOnce(Return(SenderPacketRouter::kNever));
  // Advance to the resume time. Nothing should happen until RequestRtpSend() is
  // called.
  const Clock::time_point resume_time = start_time + milliseconds(100);
  AdvanceClockAndRunTasks(resume_time - env()->now());
  router()->RequestRtpSend(kVideoReceiverSsrc);
  // Execute seventh burst.
  RunTasksUntilIdle();
  // Run for one more second, but nothing should be happening since sending is
  // paused.
  AdvanceClockAndRunTasks(seconds(1));
  Mock::VerifyAndClear(video_sender());

  // Confirm 15 packets got sent and contain the expected data (which tracks
  // when they were sent).
  ASSERT_EQ(15, static_cast<int>(packets_sent.size()));
  Clock::time_point expected_time;
  int packet_idx = 0;
  // First burst through fourth burst.
  for (int burst_number = 0; burst_number < 4; ++burst_number) {
    expected_time = start_time + burst_number * kBurstInterval;
    EXPECT_EQ(expected_time, ParseTimestamp(packets_sent[packet_idx++]));
    EXPECT_EQ(expected_time, ParseTimestamp(packets_sent[packet_idx++]));
    EXPECT_EQ(expected_time, ParseTimestamp(packets_sent[packet_idx++]));
  }
  // Fifth burst.
  expected_time += kBurstInterval;
  EXPECT_EQ(expected_time, ParseTimestamp(packets_sent[packet_idx++]));
  EXPECT_EQ(expected_time, ParseTimestamp(packets_sent[packet_idx++]));
  // Seventh burst (sixth burst sent nothing).
  EXPECT_EQ(resume_time, ParseTimestamp(packets_sent[packet_idx++]));

  router()->OnSenderDestroyed(kVideoReceiverSsrc);
}

// Tests that the SenderPacketRouter schedules packet sends based on transmit
// prority: RTCP before RTP, and the audio Sender's packets before the video
// Sender's.
TEST_F(SenderPacketRouterTest, SchedulesAndTransmitsAccountingForPriority) {
  env()->set_remote_endpoint(kRemoteEndpoint);
  ASSERT_LT(ComparePriority(kAudioReceiverSsrc, kVideoReceiverSsrc), 0);
  router()->OnSenderCreated(kVideoReceiverSsrc, video_sender());
  router()->OnSenderCreated(kAudioReceiverSsrc, audio_sender());

  // Capture every packet sent for analysis at the end of this test.
  std::vector<std::vector<uint8_t>> packets_sent;
  EXPECT_CALL(*env(), SendPacket(_))
      .WillRepeatedly(Invoke([&](absl::Span<const uint8_t> packet) {
        packets_sent.emplace_back(packet.begin(), packet.end());
      }));

  // These indicate how often one packet will be sent from each Sender.
  constexpr Clock::duration kAudioRtpInterval = milliseconds(10);
  constexpr Clock::duration kVideoRtpInterval = milliseconds(33);

  // Note: The priority flags used in this test ('0'..'3') indicate
  // lowest-to-highest priority.
  EXPECT_CALL(*audio_sender(), GetRtcpPacketForImmediateSend(_, _))
      .WillRepeatedly(
          Invoke([](Clock::time_point send_time, absl::Span<uint8_t> buffer) {
            return MakeFakePacketWithFlag('3', send_time, buffer);
          }));
  int num_audio_get_rtp_calls = 0;
  EXPECT_CALL(*audio_sender(), GetRtpPacketForImmediateSend(_, _))
      .WillRepeatedly(
          Invoke([&](Clock::time_point send_time, absl::Span<uint8_t> buffer) {
            // Alternate between returning a single packet and a "done for now"
            // signal.
            ++num_audio_get_rtp_calls;
            if (num_audio_get_rtp_calls % 2) {
              return MakeFakePacketWithFlag('1', send_time, buffer);
            }
            return buffer.subspan(0, 0);
          }));
  EXPECT_CALL(*video_sender(), GetRtcpPacketForImmediateSend(_, _))
      .WillRepeatedly(
          Invoke([](Clock::time_point send_time, absl::Span<uint8_t> buffer) {
            return MakeFakePacketWithFlag('2', send_time, buffer);
          }));
  int num_video_get_rtp_calls = 0;
  EXPECT_CALL(*video_sender(), GetRtpPacketForImmediateSend(_, _))
      .WillRepeatedly(
          Invoke([&](Clock::time_point send_time, absl::Span<uint8_t> buffer) {
            // Alternate between returning a single packet and a "done for now"
            // signal.
            ++num_video_get_rtp_calls;
            if (num_video_get_rtp_calls % 2) {
              return MakeFakePacketWithFlag('0', send_time, buffer);
            }
            return buffer.subspan(0, 0);
          }));
  EXPECT_CALL(*audio_sender(), GetRtpResumeTime()).WillRepeatedly(Invoke([&] {
    return env()->now() + kAudioRtpInterval;
  }));
  EXPECT_CALL(*video_sender(), GetRtpResumeTime()).WillRepeatedly(Invoke([&] {
    return env()->now() + kVideoRtpInterval;
  }));

  // Request starting both RTCP and RTP sends for both Senders, in a random
  // order.
  router()->RequestRtcpSend(kVideoReceiverSsrc);
  router()->RequestRtpSend(kAudioReceiverSsrc);
  router()->RequestRtcpSend(kAudioReceiverSsrc);
  router()->RequestRtpSend(kVideoReceiverSsrc);

  // Run the SenderPacketRouter for 3 seconds.
  constexpr Clock::duration kSimulationDuration = seconds(3);
  constexpr Clock::duration kSimulationStepPeriod = milliseconds(1);
  const Clock::time_point start_time = env()->now();
  RunTasksUntilIdle();
  const Clock::time_point end_time = start_time + kSimulationDuration;
  while (env()->now() <= end_time) {
    AdvanceClockAndRunTasks(kSimulationStepPeriod);
  }

  // Examine the packets that were actually sent, and confirm that the priority
  // ordering was maintained.
  ASSERT_EQ(384, static_cast<int>(packets_sent.size()));
  // The very first packet sent should be an audio RTCP packet.
  EXPECT_EQ('3', ParseFlag(packets_sent[0]));
  EXPECT_EQ(start_time, ParseTimestamp(packets_sent[0]));
  // Scan the rest, checking that packets sent in the same burst (i.e., having
  // the same send timestamp) were sent in priority order.
  char last_priority_flag = '3';
  Clock::time_point last_timestamp = start_time;
  for (int i = 1; i < static_cast<int>(packets_sent.size()) &&
                  !testing::Test::HasFailure();
       ++i) {
    const char priority_flag = ParseFlag(packets_sent[i]);
    const Clock::time_point timestamp = ParseTimestamp(packets_sent[i]);
    EXPECT_LE(last_timestamp, timestamp) << "packet[" << i << ']';
    if (timestamp == last_timestamp) {
      EXPECT_GT(last_priority_flag, priority_flag) << "packet[" << i << ']';
    }
    last_priority_flag = priority_flag;
    last_timestamp = timestamp;
  }

  router()->OnSenderDestroyed(kVideoReceiverSsrc);
  router()->OnSenderDestroyed(kAudioReceiverSsrc);
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
