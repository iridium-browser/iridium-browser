// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/frame_collector.h"

#include <stdint.h>

#include <algorithm>
#include <vector>

#include "cast/streaming/encoded_frame.h"
#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtcp_common.h"
#include "cast/streaming/rtp_time.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace cast {
namespace {

const FrameId kSomeFrameId = FrameId::first() + 39;
constexpr RtpTimeTicks kSomeRtpTimestamp =
    RtpTimeTicks() + RtpTimeDelta::FromTicks(90000);

// Convenience macro to check that the |collector| generates an expected set of
// NACKs.
#define EXPECT_HAS_NACKS(expected_nacks, collector) \
  do {                                              \
    std::vector<PacketNack> nacks;                  \
    (collector).GetMissingPackets(&nacks);          \
    EXPECT_EQ((expected_nacks), nacks);             \
  } while (false);

TEST(FrameCollectorTest, CollectsFrameWithOnlyOnePart) {
  FrameCollector collector;

  // Run for two frames to test that the collector can be re-used.
  for (int i = 0; i < 2; ++i) {
    const FrameId frame_id = kSomeFrameId + i;
    collector.set_frame_id(frame_id);
    EXPECT_FALSE(collector.is_complete());

    // With no packets seen yet, the collector should provide the "all packets
    // lost" NACK.
    EXPECT_HAS_NACKS((std::vector<PacketNack>{{frame_id, kAllPacketsLost}}),
                     collector);

    // Collect the single packet of the frame.
    RtpPacketParser::ParseResult part;
    part.rtp_timestamp = kSomeRtpTimestamp + (RtpTimeDelta::FromTicks(200) * i);
    part.frame_id = frame_id;
    part.packet_id = 0;
    part.max_packet_id = 0;
    if (i == 0) {
      part.is_key_frame = true;
      // Do not set |part.new_playout_delay|.
    } else {
      part.is_key_frame = false;
      part.new_playout_delay = std::chrono::milliseconds(800);
    }
    part.referenced_frame_id = kSomeFrameId;
    std::vector<uint8_t> buffer(255);
    for (int j = 0; j < 255; ++j) {
      buffer[j] = static_cast<uint8_t>(j);
    }
    part.payload = absl::Span<uint8_t>(buffer);
    EXPECT_TRUE(collector.CollectRtpPacket(part, &buffer));

    // At this point, the collector should feel complete.
    EXPECT_TRUE(collector.is_complete());
    EXPECT_HAS_NACKS(std::vector<PacketNack>(), collector);

    // Examine the assembled frame, and confirm its metadata and payload match
    // what was put into the collector via the packet above.
    const auto& frame = collector.PeekAtAssembledFrame();
    if (i == 0) {
      EXPECT_EQ(EncodedFrame::KEY_FRAME, frame.dependency);
      EXPECT_EQ(std::chrono::milliseconds(), frame.new_playout_delay);
    } else {
      EXPECT_EQ(EncodedFrame::DEPENDS_ON_ANOTHER, frame.dependency);
      EXPECT_EQ(std::chrono::milliseconds(800), frame.new_playout_delay);
    }
    EXPECT_EQ(part.frame_id, frame.frame_id);
    EXPECT_EQ(kSomeFrameId, frame.referenced_frame_id);
    EXPECT_EQ(part.rtp_timestamp, frame.rtp_timestamp);
    for (int j = 0; j < 255; ++j) {
      EXPECT_EQ(static_cast<uint8_t>(j), frame.data[j]);
    }

    collector.Reset();
  }
}

TEST(FrameCollectorTest, CollectsFrameWithMultiplePartsArrivingOutOfOrder) {
  FrameCollector collector;
  collector.set_frame_id(kSomeFrameId);

  // With no packets seen yet, the collector should provide the "all packets
  // lost" NACK.
  EXPECT_HAS_NACKS((std::vector<PacketNack>{{kSomeFrameId, kAllPacketsLost}}),
                   collector);

  // Prepare the six packet payloads, and the list of remaining NACKs (checked
  // after each part is collected).
  const int kPayloadSizes[] = {999, 998, 998, 998, 42, 0};
  std::vector<uint8_t> payloads[6];
  std::vector<PacketNack> remaining_nacks;
  for (int i = 0; i < 6; ++i) {
    payloads[i].resize(kPayloadSizes[i], static_cast<uint8_t>(i));
    remaining_nacks.push_back(
        PacketNack{kSomeFrameId, static_cast<FramePacketId>(i)});
  }

  // Collect all six packets, out-of-order, and with some duplicates.
  constexpr FramePacketId kPacketIds[] = {2, 0, 1, 2, 4, 3, 5, 5, 5, 0};
  for (FramePacketId packet_id : kPacketIds) {
    RtpPacketParser::ParseResult part{};
    part.rtp_timestamp = kSomeRtpTimestamp;
    part.is_key_frame = true;
    part.frame_id = kSomeFrameId;
    part.packet_id = packet_id;
    part.max_packet_id = 5;
    part.referenced_frame_id = kSomeFrameId;
    // Prepare a copy of the payload to pass into the FrameCollector. Place 24
    // bytes of bogus data at the start of the buffer to simulate the
    // non-payload part of the RTP packet.
    std::vector<uint8_t> buffer(24, uint8_t{0xab});
    buffer.insert(buffer.end(), payloads[packet_id].begin(),
                  payloads[packet_id].end());
    part.payload = absl::Span<uint8_t>(buffer.data() + 24, buffer.size() - 24);
    EXPECT_TRUE(collector.CollectRtpPacket(part, &buffer));

    // Remove the packet from the list of expected remaining NACKs, and then
    // check that the collector agrees.
    remaining_nacks.erase(
        std::remove_if(remaining_nacks.begin(), remaining_nacks.end(),
                       [packet_id](const PacketNack& nack) {
                         return nack.packet_id == packet_id;
                       }),
        remaining_nacks.end());
    EXPECT_HAS_NACKS(remaining_nacks, collector);
  }

  // Confirm there are no missing packets and no NACKs generated.
  EXPECT_TRUE(collector.is_complete());
  EXPECT_HAS_NACKS(std::vector<PacketNack>(), collector);

  // Examine the assembled frame, and confirm its metadata and payload match
  // what was put into the collector via the packets above, and that the payload
  // bytes are in-order.
  const auto& frame = collector.PeekAtAssembledFrame();
  EXPECT_EQ(EncodedFrame::KEY_FRAME, frame.dependency);
  EXPECT_EQ(kSomeFrameId, frame.frame_id);
  EXPECT_EQ(kSomeFrameId, frame.referenced_frame_id);
  EXPECT_EQ(kSomeRtpTimestamp, frame.rtp_timestamp);
  absl::Span<const uint8_t> remaining_data = frame.data;
  for (int i = 0; i < 6; ++i) {
    ASSERT_LE(kPayloadSizes[i], static_cast<int>(remaining_data.size()));
    EXPECT_EQ(absl::Span<const uint8_t>(payloads[i]),
              remaining_data.subspan(0, kPayloadSizes[i]))
        << "i=" << i;
    remaining_data.remove_prefix(kPayloadSizes[i]);
  }
  ASSERT_TRUE(remaining_data.empty());
}

TEST(FrameCollectorTest, RejectsInvalidParts) {
  FrameCollector collector;

  // Expect the collector rejects a part not having the correct FrameId.
  collector.set_frame_id(kSomeFrameId + 256);
  RtpPacketParser::ParseResult part{};
  part.rtp_timestamp = kSomeRtpTimestamp;
  part.is_key_frame = false;
  part.frame_id = kSomeFrameId;
  part.packet_id = 0;
  part.max_packet_id = 3;
  std::vector<uint8_t> buffer(1, 'A');
  part.payload = absl::Span<uint8_t>(buffer);
  EXPECT_FALSE(collector.CollectRtpPacket(part, &buffer));
  // Note: When CollectRtpPacket() returns false, it does not take ownership of
  // the buffer memory.
  ASSERT_TRUE(buffer.size() == 1 && buffer[0] == 'A');

  // The collector should accept a part having the correct FrameId.
  collector.set_frame_id(kSomeFrameId);
  part.frame_id = kSomeFrameId;
  EXPECT_TRUE(collector.CollectRtpPacket(part, &buffer));
  // Note: Re-assign the buffer and payload pointer since the buffer was just
  // consumed.
  buffer.assign(1, 'A');
  part.payload = absl::Span<uint8_t>(buffer);

  // The collector should reject a part where the packet_id is greater than the
  // previously-established max_packet_id.
  part.packet_id = 5;  // BAD, since max_packet_id is 3 (see above).
  EXPECT_FALSE(collector.CollectRtpPacket(part, &buffer));
  ASSERT_TRUE(buffer.size() == 1 && buffer[0] == 'A');

  // The collector should reject a part where the max_packet_id disagrees with
  // previously-established max_packet_id.
  part.packet_id = 2;
  part.max_packet_id = 5;  // BAD, since max_packet_id is 3 (see above).
  EXPECT_FALSE(collector.CollectRtpPacket(part, &buffer));
  ASSERT_TRUE(buffer.size() == 1 && buffer[0] == 'A');
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
