// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_COMPOUND_RTCP_BUILDER_H_
#define CAST_STREAMING_COMPOUND_RTCP_BUILDER_H_

#include <chrono>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtcp_common.h"
#include "cast/streaming/rtp_defines.h"

namespace openscreen {
namespace cast {

class RtcpSession;

// Collects current status and feedback messages from the Receiver in the
// current process, and builds compound RTCP packets to be transmitted to a
// Sender.
//
// Usage:
//
//   1. Call the various SetXYZ/IncludeXYZInNextPacket() methods as the
//      receiver's state changes. The SetXYZ() methods provide values that will
//      be included in every RTCP packet until they are changed, while the
//      IncludeXYZInNextPacket() methods provide values for only the next-built
//      RTCP packet. The latter case is part of the overall protocol design, to
//      help prevent the Sender from acting on stale Receiver state.
//
//   2. At certain times, call BuildPacket() and transmit it to the sender:
//      a. By default, every 1/2 sec, to provide the sender with a "keep alive"
//         ping that it can also use to monitor network round-trip times.
//      b. When there is new feedback, the collected information should be
//         immediately conveyed to the sender.
class CompoundRtcpBuilder {
 public:
  explicit CompoundRtcpBuilder(RtcpSession* session);
  ~CompoundRtcpBuilder();

  // Gets/Sets the checkpoint |frame_id| that will be included in built RTCP
  // packets. This value indicates to the sender that all of the packets for all
  // frames up to and including the given frame have been successfully received.
  FrameId checkpoint_frame() const { return checkpoint_frame_id_; }
  void SetCheckpointFrame(FrameId frame_id);

  // Gets/Sets the current end-to-end target playout delay setting for the Cast
  // RTP receiver, to be included in built RTCP packets. This reflect any
  // changes the sender has made by using the "Cast Adaptive Latency Extension"
  // in received RTP packets.
  std::chrono::milliseconds playout_delay() const { return playout_delay_; }
  void SetPlayoutDelay(std::chrono::milliseconds delay);

  // Gets/Sets the picture loss indicator flag. While this is set, built RTCP
  // packets will include a PLI message that indicates to the sender that there
  // has been an unrecoverable decoding error. This asks the sender to provide a
  // key frame as soon as possible. The client must explicitly clear this flag
  // when decoding will recover.
  bool is_picture_loss_indicator_set() const { return picture_loss_indicator_; }
  void SetPictureLossIndicator(bool picture_is_lost);

  // Include a receiver report about recent packet receive activity in ONLY the
  // next built RTCP packet. This replaces a prior receiver report if
  // BuildPacket() was not called in the meantime (since only the most
  // up-to-date version of the Receiver's state is relevant to the Sender).
  void IncludeReceiverReportInNextPacket(
      const RtcpReportBlock& receiver_report);

  // Include detailed feedback about wholly-received frames, whole missing
  // frames, and partially-received frames (specific missing packets) in ONLY
  // the next built RTCP packet. The data will be included in a best-effort
  // fashion, depending on the size of the |buffer| passed to the next call to
  // BuildPacket(). This replaces prior feedback data if BuildPacket() was not
  // called in the meantime (since only the most up-to-date version of the
  // Receiver's state is relevant to the Sender).
  //
  // The elements in the lists are assumed to be monotonically increasing:
  // |packet_nacks| indicates specific packets that have not yet been received,
  // or may use kAllPacketsLost to indicate that no packets have been received
  // for a frame. |frame_acks| indicates which frames after the checkpoint frame
  // have been fully received.
  void IncludeFeedbackInNextPacket(std::vector<PacketNack> packet_nacks,
                                   std::vector<FrameId> frame_acks);

  // Builds a compound RTCP packet and returns the portion of the |buffer| that
  // was used. The buffer's size must be at least kRequiredBufferSize, but
  // should generally be the maximum packet size (see discussion in
  // rtp_defines.h), to avoid dropping any ACK/NACK feedback.
  //
  // |send_time| specifies the when the resulting packet will be sent. This
  // should be monotonically increasing so the consuming side (the Sender) can
  // determine the chronological ordering of RTCP packets. The Sender might also
  // use this to estimate round-trip times over the network.
  absl::Span<uint8_t> BuildPacket(Clock::time_point send_time,
                                  absl::Span<uint8_t> buffer);

  // The required buffer size to be provided to BuildPacket(). This accounts for
  // all the possible headers and report structures that might be included,
  // along with a reasonable amount of space for the feedback's ACK/NACKs bit
  // vectors.
  static constexpr int kRequiredBufferSize = 256;

 private:
  // Helper methods called by BuildPacket() to append one RTCP packet to the
  // |buffer| that will ultimately contain a "compound RTCP packet."
  void AppendReceiverReportPacket(absl::Span<uint8_t>* buffer);
  void AppendReceiverReferenceTimeReportPacket(Clock::time_point send_time,
                                               absl::Span<uint8_t>* buffer);
  void AppendPictureLossIndicatorPacket(absl::Span<uint8_t>* buffer);
  void AppendCastFeedbackPacket(absl::Span<uint8_t>* buffer);
  int AppendCastFeedbackLossFields(absl::Span<uint8_t>* buffer);
  void AppendCastFeedbackAckFields(absl::Span<uint8_t>* buffer);

  RtcpSession* const session_;

  // Data to include in the next built RTCP packet.
  FrameId checkpoint_frame_id_ = FrameId::leader();
  std::chrono::milliseconds playout_delay_ = kDefaultTargetPlayoutDelay;
  absl::optional<RtcpReportBlock> receiver_report_for_next_packet_;
  std::vector<PacketNack> nacks_for_next_packet_;
  std::vector<FrameId> acks_for_next_packet_;
  bool picture_loss_indicator_ = false;

  // An 8-bit wrap-around counter that tracks how many times Cast Feedback has
  // been included in the built RTCP packets.
  uint8_t feedback_count_ = 0;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_COMPOUND_RTCP_BUILDER_H_
