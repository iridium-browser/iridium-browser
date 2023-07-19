// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/compound_rtcp_builder.h"

#include <algorithm>
#include <iterator>
#include <limits>

#include "cast/streaming/packet_util.h"
#include "cast/streaming/rtcp_session.h"
#include "util/integer_division.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {

CompoundRtcpBuilder::CompoundRtcpBuilder(RtcpSession* session)
    : session_(session) {
  OSP_DCHECK(session_);
}

CompoundRtcpBuilder::~CompoundRtcpBuilder() = default;

void CompoundRtcpBuilder::SetCheckpointFrame(FrameId frame_id) {
  OSP_DCHECK_GE(frame_id, checkpoint_frame_id_);
  checkpoint_frame_id_ = frame_id;
}

void CompoundRtcpBuilder::SetPlayoutDelay(std::chrono::milliseconds delay) {
  playout_delay_ = delay;
}

void CompoundRtcpBuilder::SetPictureLossIndicator(bool picture_is_lost) {
  picture_loss_indicator_ = picture_is_lost;
}

void CompoundRtcpBuilder::IncludeReceiverReportInNextPacket(
    const RtcpReportBlock& receiver_report) {
  receiver_report_for_next_packet_ = receiver_report;
}

void CompoundRtcpBuilder::IncludeFeedbackInNextPacket(
    std::vector<PacketNack> packet_nacks,
    std::vector<FrameId> frame_acks) {
  // Note: Serialization of these lists will depend on the value of
  // |checkpoint_frame_id_| when BuildPacket() is called later.

  nacks_for_next_packet_ = std::move(packet_nacks);
  acks_for_next_packet_ = std::move(frame_acks);

#if OSP_DCHECK_IS_ON()
  OSP_DCHECK(AreElementsSortedAndUnique(nacks_for_next_packet_));
  OSP_DCHECK(AreElementsSortedAndUnique(acks_for_next_packet_));

  // Consistency-check: An ACKed frame should not also be NACKed.
  for (size_t ack_i = 0, nack_i = 0; ack_i < acks_for_next_packet_.size() &&
                                     nack_i < nacks_for_next_packet_.size();) {
    const FrameId ack_frame_id = acks_for_next_packet_[ack_i];
    const FrameId nack_frame_id = nacks_for_next_packet_[nack_i].frame_id;
    if (ack_frame_id < nack_frame_id) {
      ++ack_i;
    } else if (nack_frame_id < ack_frame_id) {
      ++nack_i;
    } else {
      OSP_DCHECK_NE(ack_frame_id, nack_frame_id);
    }
  }

  // Redundancy-check: For any PacketNack whose packet ID is kAllPacketsLost,
  // there should be no other PacketNack having the same FrameId.
  for (size_t i = 1; i < nacks_for_next_packet_.size(); ++i) {
    if (nacks_for_next_packet_[i].packet_id == kAllPacketsLost) {
      // Since the elements are sorted, it's only necessary to check the
      // immediately preceeding element to make sure it does not have the same
      // FrameId.
      OSP_DCHECK_NE(nacks_for_next_packet_[i].frame_id,
                    nacks_for_next_packet_[i - 1].frame_id);
    }
  }
#endif
}

absl::Span<uint8_t> CompoundRtcpBuilder::BuildPacket(
    Clock::time_point send_time,
    absl::Span<uint8_t> buffer) {
  OSP_CHECK_GE(buffer.size(), kRequiredBufferSize);

  uint8_t* const packet_begin = buffer.data();

  // Receiver Report: Per RFC 3550, Section 6.4.2, all RTCP compound packets
  // from receivers must include at least an empty receiver report at the start.
  // It's not clear whether the Cast RTCP spec requires this, but it costs very
  // little to do so.
  AppendReceiverReportPacket(&buffer);

  // Receiver Reference Time Report: While this is optional in the Cast
  // Streaming spec, it is always included by this implementation to improve the
  // stability of the end-to-end system.
  AppendReceiverReferenceTimeReportPacket(send_time, &buffer);

  // Picture Loss Indicator: Only included if the flag is currently set.
  if (picture_loss_indicator_) {
    AppendPictureLossIndicatorPacket(&buffer);
  }

  // Cast Feedback: Checkpoint information, and add as many NACKs and ACKs as
  // the remaning space available in the buffer will allow for.
  AppendCastFeedbackPacket(&buffer);

  uint8_t* const packet_end = buffer.data();
  return absl::Span<uint8_t>(packet_begin, packet_end - packet_begin);
}

void CompoundRtcpBuilder::AppendReceiverReportPacket(
    absl::Span<uint8_t>* buffer) {
  RtcpCommonHeader header;
  header.packet_type = RtcpPacketType::kReceiverReport;
  header.payload_size = kRtcpReceiverReportSize;
  if (receiver_report_for_next_packet_) {
    header.with.report_count = 1;
    header.payload_size += kRtcpReportBlockSize;
  } else {
    header.with.report_count = 0;
  }
  header.AppendFields(buffer);
  AppendField<uint32_t>(session_->receiver_ssrc(), buffer);
  if (receiver_report_for_next_packet_) {
    receiver_report_for_next_packet_->AppendFields(buffer);
    receiver_report_for_next_packet_ = absl::nullopt;
  }
}

void CompoundRtcpBuilder::AppendReceiverReferenceTimeReportPacket(
    Clock::time_point send_time,
    absl::Span<uint8_t>* buffer) {
  RtcpCommonHeader header;
  header.packet_type = RtcpPacketType::kExtendedReports;
  header.payload_size = kRtcpExtendedReportHeaderSize +
                        kRtcpExtendedReportBlockHeaderSize +
                        kRtcpReceiverReferenceTimeReportBlockSize;
  header.AppendFields(buffer);
  AppendField<uint32_t>(session_->receiver_ssrc(), buffer);
  AppendField<uint8_t>(kRtcpReceiverReferenceTimeReportBlockType, buffer);
  AppendField<uint8_t>(0 /* reserved/unused byte */, buffer);
  AppendField<uint16_t>(
      kRtcpReceiverReferenceTimeReportBlockSize / sizeof(uint32_t), buffer);
  AppendField<uint64_t>(session_->ntp_converter().ToNtpTimestamp(send_time),
                        buffer);
}

void CompoundRtcpBuilder::AppendPictureLossIndicatorPacket(
    absl::Span<uint8_t>* buffer) {
  RtcpCommonHeader header;
  header.packet_type = RtcpPacketType::kPayloadSpecific;
  header.with.subtype = RtcpSubtype::kPictureLossIndicator;
  header.payload_size = kRtcpPictureLossIndicatorHeaderSize;
  header.AppendFields(buffer);
  AppendField<uint32_t>(session_->receiver_ssrc(), buffer);
  AppendField<uint32_t>(session_->sender_ssrc(), buffer);
}

void CompoundRtcpBuilder::AppendCastFeedbackPacket(
    absl::Span<uint8_t>* buffer) {
  // Reserve space for the RTCP Common Header. It will be serialized later,
  // after the total size of the Cast Feedback message is known.
  absl::Span<uint8_t> space_for_header =
      ReserveSpace(kRtcpCommonHeaderSize, buffer);
  uint8_t* const feedback_fields_begin = buffer->data();

  // Append the mandatory fields.
  AppendField<uint32_t>(session_->receiver_ssrc(), buffer);
  AppendField<uint32_t>(session_->sender_ssrc(), buffer);
  AppendField<uint32_t>(kRtcpCastIdentifierWord, buffer);
  AppendField<uint8_t>(checkpoint_frame_id_.lower_8_bits(), buffer);
  // The |loss_count_field| will be set after the Loss Fields are generated
  // and the total count is known.
  uint8_t* const loss_count_field =
      ReserveSpace(sizeof(uint8_t), buffer).data();
  OSP_DCHECK_GT(playout_delay_.count(), 0);
  OSP_DCHECK_LE(playout_delay_.count(), std::numeric_limits<uint16_t>::max());
  AppendField<uint16_t>(playout_delay_.count(), buffer);

  // Try to include as many Loss Fields as possible. Some of the NACKs might
  // be dropped if the remaining space in the buffer is insufficient to
  // include them all.
  const int num_loss_fields = AppendCastFeedbackLossFields(buffer);
  OSP_DCHECK_LE(num_loss_fields, std::numeric_limits<uint8_t>::max());
  *loss_count_field = num_loss_fields;

  // Try to include the CST2 header and ACK bit vector. Again, some of the
  // ACKs might be dropped if the remaining space in the buffer is
  // insufficient.
  AppendCastFeedbackAckFields(buffer);

  // Go back and fill-in the header fields, now that the total size is known.
  RtcpCommonHeader header;
  header.packet_type = RtcpPacketType::kPayloadSpecific;
  header.with.subtype = RtcpSubtype::kFeedback;
  uint8_t* const feedback_fields_end = buffer->data();
  header.payload_size = feedback_fields_end - feedback_fields_begin;
  header.AppendFields(&space_for_header);

  ++feedback_count_;
}

int CompoundRtcpBuilder::AppendCastFeedbackLossFields(
    absl::Span<uint8_t>* buffer) {
  if (nacks_for_next_packet_.empty()) {
    return 0;
  }

  // The maximum number of entries is limited by available packet buffer space
  // and the 8-bit |loss_count_field|.
  const int max_num_loss_fields =
      std::min<int>(buffer->size() / kRtcpFeedbackLossFieldSize,
                    std::numeric_limits<uint8_t>::max());

  // Translate the |nacks_for_next_packet_| list into one or more entries
  // representing specific packet losses. Omit any NACKs before the checkpoint.
  OSP_DCHECK(AreElementsSortedAndUnique(nacks_for_next_packet_));
  auto it =
      std::find_if(nacks_for_next_packet_.begin(), nacks_for_next_packet_.end(),
                   [this](const PacketNack& nack) {
                     return nack.frame_id > checkpoint_frame_id_;
                   });
  int num_loss_fields = 0;
  while (it != nacks_for_next_packet_.end() &&
         num_loss_fields != max_num_loss_fields) {
    const FrameId frame_id = it->frame_id;
    const FramePacketId first_packet_id = it->packet_id;
    uint8_t bit_vector = 0;
    for (++it; it != nacks_for_next_packet_.end() && it->frame_id == frame_id;
         ++it) {
      const int shift = it->packet_id - first_packet_id - 1;
      if (shift >= 8) {
        break;
      }
      bit_vector |= 1 << shift;
    }
    AppendField<uint8_t>(frame_id.lower_8_bits(), buffer);
    AppendField<uint16_t>(first_packet_id, buffer);
    AppendField<uint8_t>(bit_vector, buffer);
    ++num_loss_fields;
  }

  nacks_for_next_packet_.clear();
  return num_loss_fields;
}

void CompoundRtcpBuilder::AppendCastFeedbackAckFields(
    absl::Span<uint8_t>* buffer) {
  // Return if there is not enough space for the CST2 header and the
  // smallest-possible ACK bit vector.
  if (buffer->size() <
      (kRtcpFeedbackAckHeaderSize + kRtcpMinAckBitVectorOctets)) {
    return;
  }

  // Write the CST2 header and reserve/initialize the start of the ACK bit
  // vector.
  AppendField<uint32_t>(kRtcpCst2IdentifierWord, buffer);
  AppendField<uint8_t>(feedback_count_, buffer);
  // The octet count field is set later, after the total is known.
  uint8_t* const octet_count_field =
      ReserveSpace(sizeof(uint8_t), buffer).data();
  // Start with the minimum required number of bit vector octets.
  uint8_t* const ack_bitvector =
      ReserveSpace(kRtcpMinAckBitVectorOctets, buffer).data();
  int num_ack_bitvector_octets = kRtcpMinAckBitVectorOctets;
  memset(ack_bitvector, 0, kRtcpMinAckBitVectorOctets);

  // Set the bits of the ACK bit vector, auto-expanding the number of ACK octets
  // if necessary (and while there is still room in the buffer).
  if (!acks_for_next_packet_.empty()) {
    OSP_DCHECK(AreElementsSortedAndUnique(acks_for_next_packet_));
    const FrameId first_frame_id = checkpoint_frame_id_ + 2;
    for (const FrameId& frame_id : acks_for_next_packet_) {
      const int bit_index = frame_id - first_frame_id;
      if (bit_index < 0) {
        continue;
      }
      constexpr int kBitsPerOctet = 8;
      const int octet_index = bit_index / kBitsPerOctet;

      // If needed, attempt to increase the number of ACK octets.
      if (octet_index >= num_ack_bitvector_octets) {
        // Compute how many additional octets are needed.
        constexpr int kIncrement = sizeof(uint32_t);
        const int num_additional =
            DividePositivesRoundingUp(
                (octet_index + 1) - num_ack_bitvector_octets, kIncrement) *
            kIncrement;

        // If there is not enough room in the buffer to add more ACKs, then do
        // not continue. Also, if the new total count would exceed the design
        // limit, do not continue.
        if (static_cast<int>(buffer->size()) < num_additional) {
          break;
        }
        const int new_count = num_ack_bitvector_octets + num_additional;
        if (new_count > kRtcpMaxAckBitVectorOctets) {
          break;
        }

        // Reserve the additional space from the buffer, and initialize to zero.
        memset(ReserveSpace(num_additional, buffer).data(), 0, num_additional);
        num_ack_bitvector_octets = new_count;
      }

      // At this point, the ACK bit vector is valid at |octet_index|. Set the
      // bit representing the ACK for |frame_id|.
      const int shift = bit_index % kBitsPerOctet;
      ack_bitvector[octet_index] |= 1 << shift;
    }
  }

  // Now that the total size of the ACK bit vector is known, go back and set the
  // octet count field.
  OSP_DCHECK_LE(num_ack_bitvector_octets, std::numeric_limits<uint8_t>::max());
  *octet_count_field = num_ack_bitvector_octets;

  acks_for_next_packet_.clear();
}

}  // namespace cast
}  // namespace openscreen
