// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/frame_collector.h"

#include <algorithm>
#include <limits>
#include <numeric>

#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtp_defines.h"
#include "platform/base/span.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

namespace {

// Integer constant representing that the number of packets is not yet known.
constexpr int kUnknownNumberOfPackets = std::numeric_limits<int>::max();

}  // namespace

FrameCollector::FrameCollector()
    : num_missing_packets_(kUnknownNumberOfPackets) {}

FrameCollector::~FrameCollector() = default;

bool FrameCollector::CollectRtpPacket(const RtpPacketParser::ParseResult& part,
                                      std::vector<uint8_t>* buffer) {
  OSP_DCHECK(!frame_.frame_id.is_null());

  if (part.frame_id != frame_.frame_id) {
    OSP_LOG_WARN
        << "Ignoring potentially corrupt packet (frame ID mismatch). Expected: "
        << frame_.frame_id << " Got: " << part.frame_id;
    return false;
  }

  const int frame_packet_count = static_cast<int>(part.max_packet_id) + 1;
  if (num_missing_packets_ == kUnknownNumberOfPackets) {
    // This is the first packet being processed for the frame.
    num_missing_packets_ = frame_packet_count;
    chunks_.resize(num_missing_packets_);
  } else {
    // Since this is not the first packet being processed, sanity-check that the
    // "frame ID" and "max packet ID" are the expected values.
    if (frame_packet_count != static_cast<int>(chunks_.size())) {
      OSP_LOG_WARN << "Ignoring potentially corrupt packet (packet count "
                      "mismatch). packet_count="
                   << chunks_.size() << " is not equal to 1 + max_packet_id="
                   << part.max_packet_id;
      return false;
    }
  }

  // The packet ID must not be greater than the max packet ID.
  if (part.packet_id >= chunks_.size()) {
    OSP_LOG_WARN
        << "Ignoring potentially corrupt packet having invalid packet ID "
        << part.packet_id << " (should be less than " << chunks_.size() << ").";
    return false;
  }

  // Don't process duplicate packets.
  if (chunks_[part.packet_id].has_data()) {
    // Note: No logging here because this is a common occurrence that is not
    // indicative of any problem in the system.
    return true;
  }

  // Populate metadata from packet 0 only, which is the only packet that must
  // contain a complete set of values.
  if (part.packet_id == FramePacketId{0}) {
    if (part.is_key_frame) {
      frame_.dependency = EncodedFrame::Dependency::kKeyFrame;
    } else if (part.frame_id == part.referenced_frame_id) {
      frame_.dependency = EncodedFrame::Dependency::kIndependent;
    } else {
      frame_.dependency = EncodedFrame::Dependency::kDependent;
    }
    frame_.referenced_frame_id = part.referenced_frame_id;
    frame_.rtp_timestamp = part.rtp_timestamp;
    frame_.new_playout_delay = part.new_playout_delay;
  }

  // Take ownership of the contents of the |buffer| (no copy!), and record the
  // region of the buffer containing the payload data. The payload region is
  // usually all but the first few dozen bytes of the buffer.
  PayloadChunk& chunk = chunks_[part.packet_id];
  chunk.buffer.swap(*buffer);
  chunk.payload = part.payload;
  OSP_DCHECK_GE(chunk.payload.data(), chunk.buffer.data());
  OSP_DCHECK_LE(chunk.payload.data() + chunk.payload.size(),
                chunk.buffer.data() + chunk.buffer.size());

  // Success!
  --num_missing_packets_;
  OSP_DCHECK_GE(num_missing_packets_, 0);
  return true;
}

void FrameCollector::GetMissingPackets(std::vector<PacketNack>* nacks) const {
  OSP_DCHECK(!frame_.frame_id.is_null());

  if (num_missing_packets_ == 0) {
    return;
  }

  const int frame_packet_count = chunks_.size();
  if (num_missing_packets_ >= frame_packet_count) {
    nacks->push_back(PacketNack{frame_.frame_id, kAllPacketsLost});
    return;
  }

  for (int packet_id = 0; packet_id < frame_packet_count; ++packet_id) {
    if (!chunks_[packet_id].has_data()) {
      nacks->push_back(
          PacketNack{frame_.frame_id, static_cast<FramePacketId>(packet_id)});
    }
  }
}

const EncryptedFrame& FrameCollector::PeekAtAssembledFrame() {
  OSP_DCHECK_EQ(num_missing_packets_, 0);

  if (!frame_.data.data()) {
    // Allocate the frame's payload buffer once, right-sized to the sum of all
    // chunk sizes.
    frame_.owned_data_.reserve(
        std::accumulate(chunks_.cbegin(), chunks_.cend(), size_t{0},
                        [](size_t num_bytes_so_far, const PayloadChunk& chunk) {
                          return num_bytes_so_far + chunk.payload.size();
                        }));
    // Now, populate the frame's payload buffer with each chunk of data.
    for (const PayloadChunk& chunk : chunks_) {
      frame_.owned_data_.insert(frame_.owned_data_.end(), chunk.payload.begin(),
                                chunk.payload.end());
    }
    frame_.data = frame_.owned_data_;
  }

  return frame_;
}

void FrameCollector::Reset() {
  num_missing_packets_ = kUnknownNumberOfPackets;
  frame_.frame_id = FrameId();
  frame_.owned_data_.clear();
  frame_.owned_data_.shrink_to_fit();
  frame_.data = ByteView();
  chunks_.clear();
}

FrameCollector::PayloadChunk::PayloadChunk() = default;
FrameCollector::PayloadChunk::~PayloadChunk() = default;

}  // namespace cast
}  // namespace openscreen
