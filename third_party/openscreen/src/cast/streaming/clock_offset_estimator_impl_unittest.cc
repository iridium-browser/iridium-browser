// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/clock_offset_estimator_impl.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "gtest/gtest.h"
#include "platform/base/trivial_clock_traits.h"
#include "platform/test/fake_clock.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {

class ClockOffsetEstimatorImplTest : public ::testing::Test {
 public:
  ClockOffsetEstimatorImplTest()
      : sender_time_(Clock::now()), receiver_clock_(Clock::now()) {}

  ~ClockOffsetEstimatorImplTest() override = default;

  void AdvanceClocks(Clock::duration time) {
    receiver_clock_.Advance(time);
    sender_time_ += time;
  }

  Clock::time_point sender_time_;
  // Only one fake clock instance is allowed.
  FakeClock receiver_clock_;
  ClockOffsetEstimatorImpl estimator_;
};

// Suppose the true offset is 100ms.
// Event A occurred at sender time 20ms.
// Event B occurred at receiver time 130ms. (sender time 30ms)
// Event C occurred at sender time 60ms.
// Then the bound after all 3 events have arrived is [130-60=70, 130-20=110].
TEST_F(ClockOffsetEstimatorImplTest, EstimateOffset) {
  const milliseconds kTrueOffset(100);
  receiver_clock_.Advance(kTrueOffset);

  Clock::duration lower_bound;
  Clock::duration upper_bound;

  EXPECT_FALSE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));

  const RtpTimeTicks rtp_timestamp;
  FrameId frame_id = FrameId::first();

  AdvanceClocks(milliseconds(20));

  FrameEvent encode_event;
  encode_event.timestamp = sender_time_;
  encode_event.type = StatisticsEventType::kFrameEncoded;
  encode_event.media_type = StatisticsEventMediaType::kVideo;
  encode_event.rtp_timestamp = rtp_timestamp;
  encode_event.frame_id = frame_id;
  encode_event.size = 1234;
  encode_event.key_frame = true;
  encode_event.target_bitrate = 5678;
  estimator_.OnFrameEvent(encode_event);

  PacketEvent send_event;
  send_event.timestamp = sender_time_;
  send_event.type = StatisticsEventType::kPacketSentToNetwork;
  send_event.media_type = StatisticsEventMediaType::kVideo;
  send_event.rtp_timestamp = rtp_timestamp;
  send_event.frame_id = frame_id;
  send_event.packet_id = 56;
  send_event.max_packet_id = 78;
  send_event.size = 1500;
  estimator_.OnPacketEvent(send_event);

  EXPECT_FALSE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));

  AdvanceClocks(milliseconds(10));
  FrameEvent ack_sent_event;
  ack_sent_event.timestamp = receiver_clock_.now();
  ack_sent_event.type = StatisticsEventType::kFrameAckSent;
  ack_sent_event.media_type = StatisticsEventMediaType::kVideo;
  ack_sent_event.rtp_timestamp = rtp_timestamp;
  ack_sent_event.frame_id = frame_id;
  estimator_.OnFrameEvent(ack_sent_event);

  PacketEvent receive_event;
  receive_event.timestamp = receiver_clock_.now();
  receive_event.type = StatisticsEventType::kPacketReceived;
  receive_event.media_type = StatisticsEventMediaType::kVideo;
  receive_event.rtp_timestamp = rtp_timestamp;
  receive_event.frame_id = frame_id;
  receive_event.packet_id = 56;
  receive_event.max_packet_id = 78;
  receive_event.size = 1500;
  estimator_.OnPacketEvent(receive_event);

  EXPECT_FALSE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));

  AdvanceClocks(milliseconds(30));
  FrameEvent ack_event;
  ack_event.timestamp = sender_time_;
  ack_event.type = StatisticsEventType::kFrameAckReceived;
  ack_event.media_type = StatisticsEventMediaType::kVideo;
  ack_event.rtp_timestamp = rtp_timestamp;
  ack_event.frame_id = frame_id;
  estimator_.OnFrameEvent(ack_event);

  EXPECT_TRUE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));

  EXPECT_EQ(milliseconds(70), to_milliseconds(lower_bound));
  EXPECT_EQ(milliseconds(110), to_milliseconds(upper_bound));
  EXPECT_GE(kTrueOffset, lower_bound);
  EXPECT_LE(kTrueOffset, upper_bound);
}

// Same scenario as above, but event C arrives before event B. It doesn't mean
// event C occurred before event B.
TEST_F(ClockOffsetEstimatorImplTest, EventCArrivesBeforeEventB) {
  constexpr milliseconds kTrueOffset(100);
  receiver_clock_.Advance(kTrueOffset);

  Clock::duration lower_bound;
  Clock::duration upper_bound;

  EXPECT_FALSE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));

  const RtpTimeTicks rtp_timestamp;
  FrameId frame_id = FrameId::first();

  AdvanceClocks(milliseconds(20));

  FrameEvent encode_event;
  encode_event.timestamp = sender_time_;
  encode_event.type = StatisticsEventType::kFrameEncoded;
  encode_event.media_type = StatisticsEventMediaType::kVideo;
  encode_event.rtp_timestamp = rtp_timestamp;
  encode_event.frame_id = frame_id;
  encode_event.size = 1234;
  encode_event.key_frame = true;
  encode_event.target_bitrate = 5678;
  estimator_.OnFrameEvent(encode_event);

  PacketEvent send_event;
  send_event.timestamp = sender_time_;
  send_event.type = StatisticsEventType::kPacketSentToNetwork;
  send_event.media_type = StatisticsEventMediaType::kVideo;
  send_event.rtp_timestamp = rtp_timestamp;
  send_event.frame_id = frame_id;
  send_event.packet_id = 56;
  send_event.max_packet_id = 78;
  send_event.size = 1500;
  estimator_.OnPacketEvent(send_event);

  EXPECT_FALSE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));

  AdvanceClocks(milliseconds(10));
  Clock::time_point event_b_time = receiver_clock_.now();
  AdvanceClocks(milliseconds(30));
  Clock::time_point event_c_time = sender_time_;

  FrameEvent ack_event;
  ack_event.timestamp = event_c_time;
  ack_event.type = StatisticsEventType::kFrameAckReceived;
  ack_event.media_type = StatisticsEventMediaType::kVideo;
  ack_event.rtp_timestamp = rtp_timestamp;
  ack_event.frame_id = frame_id;
  estimator_.OnFrameEvent(ack_event);

  EXPECT_FALSE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));

  PacketEvent receive_event;
  receive_event.timestamp = event_b_time;
  receive_event.type = StatisticsEventType::kPacketReceived;
  receive_event.media_type = StatisticsEventMediaType::kVideo;
  receive_event.rtp_timestamp = rtp_timestamp;
  receive_event.frame_id = frame_id;
  receive_event.packet_id = 56;
  receive_event.max_packet_id = 78;
  receive_event.size = 1500;
  estimator_.OnPacketEvent(receive_event);

  FrameEvent ack_sent_event;
  ack_sent_event.timestamp = event_b_time;
  ack_sent_event.type = StatisticsEventType::kFrameAckSent;
  ack_sent_event.media_type = StatisticsEventMediaType::kVideo;
  ack_sent_event.rtp_timestamp = rtp_timestamp;
  ack_sent_event.frame_id = frame_id;
  estimator_.OnFrameEvent(ack_sent_event);

  EXPECT_TRUE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));

  // Note: although the bounds are measured in microseconds, we round here to
  // the nearest millisecond to avoid comparison inaccuracies due to floating
  // point representation.
  EXPECT_EQ(milliseconds(70), to_milliseconds(lower_bound));
  EXPECT_EQ(milliseconds(110), to_milliseconds(upper_bound));
  EXPECT_GE(kTrueOffset, lower_bound);
  EXPECT_LE(kTrueOffset, upper_bound);
}

TEST_F(ClockOffsetEstimatorImplTest, MultipleIterations) {
  constexpr milliseconds kTrueOffset(100);
  receiver_clock_.Advance(milliseconds(kTrueOffset));

  Clock::duration lower_bound;
  Clock::duration upper_bound;

  const RtpTimeTicks rtp_timestamp_a;
  FrameId frame_id_a = FrameId::first();
  const RtpTimeTicks rtp_timestamp_b =
      rtp_timestamp_a + RtpTimeDelta::FromTicks(90);
  FrameId frame_id_b = frame_id_a + 1;
  const RtpTimeTicks rtp_timestamp_c =
      rtp_timestamp_b + RtpTimeDelta::FromTicks(90);
  FrameId frame_id_c = frame_id_b + 1;

  // Frame 1 times: [20, 30+100, 60]
  // Frame 2 times: [30, 50+100, 55]
  // Frame 3 times: [77, 80+100, 110]
  // Bound should end up at [95, 103]
  // Events times in chronological order: 20, 30 x2, 50, 55, 60, 77, 80, 110
  AdvanceClocks(milliseconds(20));
  FrameEvent encode_event;
  encode_event.timestamp = sender_time_;
  encode_event.type = StatisticsEventType::kFrameEncoded;
  encode_event.media_type = StatisticsEventMediaType::kVideo;
  encode_event.rtp_timestamp = rtp_timestamp_a;
  encode_event.frame_id = frame_id_a;
  encode_event.size = 1234;
  encode_event.key_frame = true;
  encode_event.target_bitrate = 5678;
  estimator_.OnFrameEvent(encode_event);

  PacketEvent send_event;
  send_event.timestamp = sender_time_;
  send_event.type = StatisticsEventType::kPacketSentToNetwork;
  send_event.media_type = StatisticsEventMediaType::kVideo;
  send_event.rtp_timestamp = rtp_timestamp_a;
  send_event.frame_id = frame_id_a;
  send_event.packet_id = 56;
  send_event.max_packet_id = 78;
  send_event.size = 1500;
  estimator_.OnPacketEvent(send_event);

  AdvanceClocks(milliseconds(10));
  FrameEvent second_encode_event;
  second_encode_event.timestamp = sender_time_;
  second_encode_event.type = StatisticsEventType::kFrameEncoded;
  second_encode_event.media_type = StatisticsEventMediaType::kVideo;
  second_encode_event.rtp_timestamp = rtp_timestamp_b;
  second_encode_event.frame_id = frame_id_b;
  second_encode_event.size = 1234;
  second_encode_event.key_frame = true;
  second_encode_event.target_bitrate = 5678;
  estimator_.OnFrameEvent(second_encode_event);

  PacketEvent second_send_event;
  second_send_event.timestamp = sender_time_;
  second_send_event.type = StatisticsEventType::kPacketSentToNetwork;
  second_send_event.media_type = StatisticsEventMediaType::kVideo;
  second_send_event.rtp_timestamp = rtp_timestamp_b;
  second_send_event.frame_id = frame_id_b;
  second_send_event.packet_id = 56;
  second_send_event.max_packet_id = 78;
  second_send_event.size = 1500;
  estimator_.OnPacketEvent(second_send_event);

  FrameEvent ack_sent_event;
  ack_sent_event.timestamp = receiver_clock_.now();
  ack_sent_event.type = StatisticsEventType::kFrameAckSent;
  ack_sent_event.media_type = StatisticsEventMediaType::kVideo;
  ack_sent_event.rtp_timestamp = rtp_timestamp_a;
  ack_sent_event.frame_id = frame_id_a;
  estimator_.OnFrameEvent(ack_sent_event);

  AdvanceClocks(milliseconds(20));

  PacketEvent receive_event;
  receive_event.timestamp = receiver_clock_.now();
  receive_event.type = StatisticsEventType::kPacketReceived;
  receive_event.media_type = StatisticsEventMediaType::kVideo;
  receive_event.rtp_timestamp = rtp_timestamp_b;
  receive_event.frame_id = frame_id_b;
  receive_event.packet_id = 56;
  receive_event.max_packet_id = 78;
  receive_event.size = 1500;
  estimator_.OnPacketEvent(receive_event);

  FrameEvent second_ack_sent_event;
  second_ack_sent_event.timestamp = receiver_clock_.now();
  second_ack_sent_event.type = StatisticsEventType::kFrameAckSent;
  second_ack_sent_event.media_type = StatisticsEventMediaType::kVideo;
  second_ack_sent_event.rtp_timestamp = rtp_timestamp_b;
  second_ack_sent_event.frame_id = frame_id_b;
  estimator_.OnFrameEvent(second_ack_sent_event);

  AdvanceClocks(milliseconds(5));
  FrameEvent ack_event;
  ack_event.timestamp = sender_time_;
  ack_event.type = StatisticsEventType::kFrameAckReceived;
  ack_event.media_type = StatisticsEventMediaType::kVideo;
  ack_event.rtp_timestamp = rtp_timestamp_b;
  ack_event.frame_id = frame_id_b;
  estimator_.OnFrameEvent(ack_event);

  AdvanceClocks(milliseconds(5));
  FrameEvent second_ack_event;
  second_ack_event.timestamp = sender_time_;
  second_ack_event.type = StatisticsEventType::kFrameAckReceived;
  second_ack_event.media_type = StatisticsEventMediaType::kVideo;
  second_ack_event.rtp_timestamp = rtp_timestamp_a;
  second_ack_event.frame_id = frame_id_a;
  estimator_.OnFrameEvent(second_ack_event);

  AdvanceClocks(milliseconds(17));
  FrameEvent third_encode_event;
  third_encode_event.timestamp = sender_time_;
  third_encode_event.type = StatisticsEventType::kFrameEncoded;
  third_encode_event.media_type = StatisticsEventMediaType::kVideo;
  third_encode_event.rtp_timestamp = rtp_timestamp_c;
  third_encode_event.frame_id = frame_id_c;
  third_encode_event.size = 1234;
  third_encode_event.key_frame = true;
  third_encode_event.target_bitrate = 5678;
  estimator_.OnFrameEvent(third_encode_event);

  PacketEvent third_send_event;
  third_send_event.timestamp = sender_time_;
  third_send_event.type = StatisticsEventType::kPacketSentToNetwork;
  third_send_event.media_type = StatisticsEventMediaType::kVideo;
  third_send_event.rtp_timestamp = rtp_timestamp_c;
  third_send_event.frame_id = frame_id_c;
  third_send_event.packet_id = 56;
  third_send_event.max_packet_id = 78;
  third_send_event.size = 1500;
  estimator_.OnPacketEvent(third_send_event);

  AdvanceClocks(milliseconds(3));
  PacketEvent second_receive_event;
  second_receive_event.timestamp = receiver_clock_.now();
  second_receive_event.type = StatisticsEventType::kPacketReceived;
  second_receive_event.media_type = StatisticsEventMediaType::kVideo;
  second_receive_event.rtp_timestamp = rtp_timestamp_c;
  second_receive_event.frame_id = frame_id_c;
  second_receive_event.packet_id = 56;
  second_receive_event.max_packet_id = 78;
  second_receive_event.size = 1500;
  estimator_.OnPacketEvent(second_receive_event);

  FrameEvent third_ack_sent_event;
  third_ack_sent_event.timestamp = receiver_clock_.now();
  third_ack_sent_event.type = StatisticsEventType::kFrameAckSent;
  third_ack_sent_event.media_type = StatisticsEventMediaType::kVideo;
  third_ack_sent_event.rtp_timestamp = rtp_timestamp_c;
  third_ack_sent_event.frame_id = frame_id_c;
  estimator_.OnFrameEvent(third_ack_sent_event);

  AdvanceClocks(milliseconds(30));
  FrameEvent third_ack_event;
  third_ack_event.timestamp = sender_time_;
  third_ack_event.type = StatisticsEventType::kFrameAckReceived;
  third_ack_event.media_type = StatisticsEventMediaType::kVideo;
  third_ack_event.rtp_timestamp = rtp_timestamp_c;
  third_ack_event.frame_id = frame_id_c;
  estimator_.OnFrameEvent(third_ack_event);

  EXPECT_TRUE(estimator_.GetReceiverOffsetBounds(lower_bound, upper_bound));
  EXPECT_GT(lower_bound, milliseconds(90));
  EXPECT_LE(lower_bound, kTrueOffset);
  EXPECT_LT(upper_bound, milliseconds(150));
  EXPECT_GT(upper_bound, kTrueOffset);
}

}  // namespace cast
}  // namespace openscreen
