// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_FRAME_COLLECTOR_H_
#define CAST_STREAMING_FRAME_COLLECTOR_H_

#include <vector>

#include "absl/types/span.h"
#include "cast/streaming/frame_crypto.h"
#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtcp_common.h"
#include "cast/streaming/rtp_packet_parser.h"

namespace openscreen {
namespace cast {

// Used by a Receiver to collect the parts of a frame, track what is
// missing/complete, and assemble a complete frame.
class FrameCollector {
 public:
  FrameCollector();
  ~FrameCollector();

  // Sets the ID of the current frame being collected. This must be called after
  // each Reset(), and before any of the other methods.
  void set_frame_id(FrameId frame_id) { frame_.frame_id = frame_id; }

  // Examine the parsed packet, representing part of the whole frame, and
  // collect any data/metadata from it that helps complete the frame. Returns
  // false if the |part| contained invalid data. On success, this method takes
  // the data contained within the |buffer|, into which |part.payload| is
  // pointing, in lieu of copying the data.
  [[nodiscard]] bool CollectRtpPacket(const RtpPacketParser::ParseResult& part,
                                      std::vector<uint8_t>* buffer);

  // Returns true if the frame data collection is complete and the frame can be
  // assembled.
  bool is_complete() const { return num_missing_packets_ == 0; }

  // Appends zero or more elements to |nacks| representing which packets are not
  // yet collected. If all packets for the frame are missing, this appends a
  // single element containing the special kAllPacketsLost packet ID. Otherwise,
  // one element is appended for each missing packet, in increasing order of
  // packet ID.
  void GetMissingPackets(std::vector<PacketNack>* nacks) const;

  // Returns a read-only reference to the completely-collected frame, assembling
  // it if necessary. The caller should reset the FrameCollector (see Reset()
  // below) to free-up memory once it has finished reading from the returned
  // frame.
  //
  // Precondition: is_complete() must return true before this method can be
  // called.
  const EncryptedFrame& PeekAtAssembledFrame();

  // Resets the FrameCollector back to its initial state, freeing-up memory.
  void Reset();

 private:
  struct PayloadChunk {
    std::vector<uint8_t> buffer;
    absl::Span<const uint8_t> payload;  // Once set, is within |buffer.data()|.

    PayloadChunk();
    ~PayloadChunk();

    bool has_data() const { return !!payload.data(); }
  };

  // Storage for frame metadata and data. Once the frame has been completely
  // collected and assembled, |frame_.data| is set to non-null, and this is
  // exposed externally (read-only).
  EncryptedFrame frame_;

  // The number of packets needed to complete the frame, or the maximum int if
  // this is not yet known.
  int num_missing_packets_;

  // The chunks of payload data being collected, where element indices
  // correspond 1:1 with packet IDs. When the first part is collected, this is
  // resized to match the total number of packets being expected.
  std::vector<PayloadChunk> chunks_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_FRAME_COLLECTOR_H_
