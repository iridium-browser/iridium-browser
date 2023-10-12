// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/compound_rtcp_parser.h"

#include <algorithm>
#include <utility>

#include "cast/streaming/packet_util.h"
#include "cast/streaming/rtcp_session.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {

namespace {

// Use the Clock's minimum time value (an impossible value, waaaaay before epoch
// time) to represent unset time_point values.
constexpr auto kNullTimePoint = Clock::time_point::min();

constexpr uint32_t kCastName = ('C' << 24) + ('A' << 16) + ('S' << 8) + 'T';

// Some receivers send time sync requests (that we ignore).
constexpr uint32_t kTimeSyncRequestName =
    ('T' << 24) + ('I' << 16) + ('M' << 8) + 'E';

// Canonicalizes the just-parsed list of packet-specific NACKs so that the
// CompoundRtcpParser::Client can make several simplifying assumptions when
// processing the results.
void CanonicalizePacketNackVector(std::vector<PacketNack>* packets) {
  // First, sort all elements. The sort order is the normal lexicographical
  // ordering, with one exception: The special kAllPacketsLost packet_id value
  // should be treated as coming before all others. This special sort order
  // allows the filtering algorithm below to be simpler, and only require one
  // pass; and the final result will be the normal lexicographically-sorted
  // output the CompoundRtcpParser::Client expects.
  std::sort(packets->begin(), packets->end(),
            [](const PacketNack& a, const PacketNack& b) {
              // Since the comparator is a hot code path, use a simple modular
              // arithmetic trick in lieu of extra branching: When comparing the
              // tuples, map all packet_id values to packet_id + 1, mod 0x10000.
              // This results in the desired sorting behavior since
              // kAllPacketsLost (0xffff) wraps-around to 0x0000, and all other
              // values become N + 1.
              static_assert(static_cast<FramePacketId>(kAllPacketsLost + 1) <
                                FramePacketId{0x0000 + 1},
                            "comparison requires integer wrap-around");
              return PacketNack{a.frame_id,
                                static_cast<FramePacketId>(a.packet_id + 1)} <
                     PacketNack{b.frame_id,
                                static_cast<FramePacketId>(b.packet_id + 1)};
            });

  // De-duplicate elements. Two possible cases:
  //
  //   1. Identical elements (same FrameId+FramePacketId).
  //   2. If there are any elements with kAllPacketsLost as the packet ID,
  //      prune-out all other elements having the same frame ID, as they are
  //      redundant.
  //
  // This is done by walking forwards over the sorted vector and deciding which
  // elements to keep. Those that are kept are stacked-up at the front of the
  // vector. After the "to-keep" pass, the vector is truncated to remove the
  // left-over garbage at the end.
  auto have_it = packets->begin();
  if (have_it != packets->end()) {
    auto kept_it = have_it;  // Always keep the first element.
    for (++have_it; have_it != packets->end(); ++have_it) {
      if (have_it->frame_id != kept_it->frame_id ||
          (kept_it->packet_id != kAllPacketsLost &&
           have_it->packet_id != kept_it->packet_id)) {  // Keep it.
        ++kept_it;
        *kept_it = *have_it;
      }
    }
    packets->erase(++kept_it, packets->end());
  }
}

// TODO(issuetracker.google.com/298085631): implement the serialization of
// StatisticsEventType to wire type as part of implementing receiver side event
// generation.
// NOTE: the legacy mappings, like AudioAckSent below, may still be in use
// on some legacy receivers.
StatisticsEventType ToEventTypeFromWire(uint8_t wire_event) {
  switch (wire_event) {
    case 1:   // AudioAckSent
    case 5:   // VideoAckSent
    case 11:  // Unified
      return StatisticsEventType::kFrameAckSent;

    case 2:   // AudioPlayoutDelay
    case 7:   // VideoRenderDelay
    case 12:  // Unified
      return StatisticsEventType::kFramePlayedOut;

    case 3:   // AudioFrameDecoded
    case 6:   // VideoFrameDecoded
    case 13:  // Unified
      return StatisticsEventType::kFrameDecoded;

    case 4:   // AudioPacketReceived
    case 8:   // VideoPacketReceived
    case 14:  // Unified
      return StatisticsEventType::kPacketReceived;

    default:
      OSP_VLOG << "Unexpected RTCP log message received: "
               << static_cast<int>(wire_event);
      return StatisticsEventType::kUnknown;
  }
}

}  // namespace

CompoundRtcpParser::CompoundRtcpParser(RtcpSession* session,
                                       CompoundRtcpParser::Client* client)
    : session_(session),
      client_(client),
      latest_receiver_timestamp_(kNullTimePoint) {
  OSP_DCHECK(session_);
  OSP_DCHECK(client_);
}

CompoundRtcpParser::~CompoundRtcpParser() = default;

bool CompoundRtcpParser::Parse(ByteView buffer, FrameId max_feedback_frame_id) {
  // These will contain the results from the various ParseXYZ() methods. None of
  // the results will be dispatched to the Client until the entire parse
  // succeeds.
  Clock::time_point receiver_reference_time = kNullTimePoint;
  absl::optional<RtcpReportBlock> receiver_report;
  std::vector<RtcpReceiverFrameLogMessage> log_messages;
  FrameId checkpoint_frame_id;
  milliseconds target_playout_delay{};
  std::vector<FrameId> received_frames;
  std::vector<PacketNack> packet_nacks;
  bool picture_loss_indicator = false;

  // The data contained in |buffer| can be a "compound packet," which means that
  // it can be the concatenation of multiple RTCP packets. The loop here
  // processes each one-by-one.
  while (!buffer.empty()) {
    const auto header = RtcpCommonHeader::Parse(buffer);
    if (!header) {
      return false;
    }
    buffer.remove_prefix(kRtcpCommonHeaderSize);
    if (static_cast<int>(buffer.size()) < header->payload_size) {
      return false;
    }
    ByteView payload = buffer.subspan(0, header->payload_size);
    buffer.remove_prefix(header->payload_size);

    switch (header->packet_type) {
      case RtcpPacketType::kReceiverReport:
        if (!ParseReceiverReport(payload, header->with.report_count,
                                 receiver_report)) {
          return false;
        }
        break;

      case RtcpPacketType::kApplicationDefined:
        if (!ParseApplicationDefined(header->with.subtype, payload,
                                     log_messages)) {
          return false;
        }
        break;

      case RtcpPacketType::kPayloadSpecific:
        switch (header->with.subtype) {
          case RtcpSubtype::kPictureLossIndicator:
            if (!ParsePictureLossIndicator(payload, picture_loss_indicator)) {
              return false;
            }
            break;
          case RtcpSubtype::kFeedback:
            if (!ParseFeedback(payload, max_feedback_frame_id,
                               &checkpoint_frame_id, &target_playout_delay,
                               &received_frames, packet_nacks)) {
              return false;
            }
            break;
          default:
            // Ignore: Unimplemented or not part of the Cast Streaming spec.
            break;
        }
        break;

      case RtcpPacketType::kExtendedReports:
        if (!ParseExtendedReports(payload, receiver_reference_time)) {
          return false;
        }
        break;

      default:
        // Ignored, unimplemented or not part of the Cast Streaming spec.
        break;
    }
  }

  // A well-behaved Cast Streaming Receiver will always include a reference time
  // report. This essentially "timestamps" the RTCP packets just parsed.
  // However, the spec does not explicitly require this be included. When it is
  // present, improve the stability of the system by ignoring stale/out-of-order
  // RTCP packets.
  if (receiver_reference_time != kNullTimePoint) {
    // If the packet is out-of-order (e.g., it got delayed/shuffled when going
    // through the network), just ignore it. Since RTCP packets always include
    // all the necessary current state from the peer, dropping them does not
    // mean important signals will be lost. In fact, it can actually be harmful
    // to process compound RTCP packets out-of-order.
    if (latest_receiver_timestamp_ != kNullTimePoint &&
        receiver_reference_time < latest_receiver_timestamp_) {
      return true;
    }
    latest_receiver_timestamp_ = receiver_reference_time;
    client_->OnReceiverReferenceTimeAdvanced(latest_receiver_timestamp_);
  }

  // At this point, the packet is known to be well-formed. Dispatch events of
  // interest to the Client.
  if (receiver_report) {
    client_->OnReceiverReport(*receiver_report);
  }
  if (!log_messages.empty()) {
    client_->OnCastReceiverFrameLogMessages(std::move(log_messages));
  }
  if (!checkpoint_frame_id.is_null()) {
    client_->OnReceiverCheckpoint(checkpoint_frame_id, target_playout_delay);
  }
  if (!received_frames.empty()) {
    OSP_DCHECK(AreElementsSortedAndUnique(received_frames));
    client_->OnReceiverHasFrames(std::move(received_frames));
  }
  CanonicalizePacketNackVector(&packet_nacks);
  if (!packet_nacks.empty()) {
    client_->OnReceiverIsMissingPackets(std::move(packet_nacks));
  }
  if (picture_loss_indicator) {
    client_->OnReceiverIndicatesPictureLoss();
  }

  return true;
}

bool CompoundRtcpParser::ParseReceiverReport(
    ByteView in,
    int num_report_blocks,
    absl::optional<RtcpReportBlock>& receiver_report) {
  if (in.size() < kRtcpReceiverReportSize) {
    return false;
  }
  if (ConsumeField<uint32_t>(in) == session_->receiver_ssrc()) {
    receiver_report = RtcpReportBlock::ParseOne(in, num_report_blocks,
                                                session_->sender_ssrc());
  }
  return true;
}

bool CompoundRtcpParser::ParseApplicationDefined(
    RtcpSubtype subtype,
    ByteView in,
    std::vector<RtcpReceiverFrameLogMessage>& messages) {
  const uint32_t sender_ssrc = ConsumeField<uint32_t>(in);
  const uint32_t name = ConsumeField<uint32_t>(in);

  // Just ignore events that aren't intended for us.
  if (sender_ssrc != session_->receiver_ssrc()) {
    return true;
  }
  if (name != kCastName) {
    // We ignore time sync requests but don't throw an error for them.
    return name == kTimeSyncRequestName;
  }
  if (subtype == RtcpSubtype::kReceiverLog) {
    return ParseFrameLogMessages(in, messages);
  }
  return true;
}

bool CompoundRtcpParser::ParseFrameLogMessages(
    ByteView in,
    std::vector<RtcpReceiverFrameLogMessage>& messages) {
  while (!in.empty()) {
    if (in.size() < kRtcpReceiverFrameLogMessageHeaderSize) {
      messages.clear();
      return false;
    }
    const uint32_t truncated_rtp_timestamp = ConsumeField<uint32_t>(in);
    const uint32_t data = ConsumeField<uint32_t>(in);

    // The 24 least significant bits contain the event timestamp, which is
    // offset from when the first packet was sent.
    const uint32_t raw_timestamp = data & 0xFFFFFF;
    const Clock::time_point event_timestamp_base =
        session_->start_time() + milliseconds(raw_timestamp);

    // The 8 most significant bits contain the number of events.
    // NOTE: at least one event is required, so a value of "0" over the wire
    // actually means there is one event.
    const size_t num_events = 1u + static_cast<uint8_t>(data >> 24);

    const RtpTimeTicks frame_log_rtp_timestamp =
        latest_frame_log_rtp_timestamp_.Expand(truncated_rtp_timestamp);
    RtcpReceiverFrameLogMessage frame_log_message{.rtp_timestamp =
                                                      frame_log_rtp_timestamp};

    for (size_t event = 0; event < num_events; ++event) {
      if (in.size() < kRtcpReceiverFrameLogMessageBlockSize) {
        messages.clear();
        return false;
      }

      const uint16_t delay_delta_or_packet_id = ConsumeField<uint16_t>(in);
      const uint16_t event_type_and_timestamp_delta =
          ConsumeField<uint16_t>(in);

      // Skip unknown event types, they are not useful.
      const StatisticsEventType event_type = ToEventTypeFromWire(
          static_cast<uint8_t>(event_type_and_timestamp_delta >> 12));
      if (event_type == StatisticsEventType::kUnknown) {
        continue;
      }

      RtcpReceiverEventLogMessage event_log{
          .type = event_type,
          .timestamp = event_timestamp_base +
                       milliseconds(event_type_and_timestamp_delta & 0xFFF)};
      if (event_type == StatisticsEventType::kPacketReceived) {
        event_log.packet_id = delay_delta_or_packet_id;
      } else {
        event_log.delay =
            milliseconds(static_cast<int16_t>(delay_delta_or_packet_id));
      }
      frame_log_message.messages.emplace_back(std::move(event_log));
    }
    latest_frame_log_rtp_timestamp_ = frame_log_rtp_timestamp;
    messages.emplace_back(std::move(frame_log_message));
  }

  return true;
}

bool CompoundRtcpParser::ParseFeedback(ByteView in,
                                       FrameId max_feedback_frame_id,
                                       FrameId* checkpoint_frame_id,
                                       milliseconds* target_playout_delay,
                                       std::vector<FrameId>* received_frames,
                                       std::vector<PacketNack>& packet_nacks) {
  OSP_DCHECK(!max_feedback_frame_id.is_null());

  if (static_cast<int>(in.size()) < kRtcpFeedbackHeaderSize) {
    return false;
  }
  if (ConsumeField<uint32_t>(in) != session_->receiver_ssrc() ||
      ConsumeField<uint32_t>(in) != session_->sender_ssrc()) {
    return true;  // Ignore report from mismatched SSRC(s).
  }
  if (ConsumeField<uint32_t>(in) != kRtcpCastIdentifierWord) {
    return false;
  }

  const FrameId feedback_frame_id =
      max_feedback_frame_id.ExpandLessThanOrEqual(ConsumeField<uint8_t>(in));
  const int loss_field_count = ConsumeField<uint8_t>(in);
  const auto playout_delay = milliseconds(ConsumeField<uint16_t>(in));
  // Don't process feedback that would move the checkpoint backwards. The Client
  // makes assumptions about what frame data and other tracking state can be
  // discarded based on a monotonically non-decreasing checkpoint FrameId.
  if (!checkpoint_frame_id->is_null() &&
      *checkpoint_frame_id > feedback_frame_id) {
    return true;
  }
  *checkpoint_frame_id = feedback_frame_id;
  *target_playout_delay = playout_delay;
  received_frames->clear();
  packet_nacks.clear();
  if (static_cast<int>(in.size()) <
      (kRtcpFeedbackLossFieldSize * loss_field_count)) {
    return false;
  }

  // Parse the NACKs.
  for (int i = 0; i < loss_field_count; ++i) {
    const FrameId frame_id =
        feedback_frame_id.ExpandGreaterThan(ConsumeField<uint8_t>(in));
    FramePacketId packet_id = ConsumeField<uint16_t>(in);
    uint8_t bits = ConsumeField<uint8_t>(in);
    packet_nacks.push_back(PacketNack{frame_id, packet_id});

    if (packet_id != kAllPacketsLost) {
      // Translate each set bit in the bit vector into another missing
      // FramePacketId.
      while (bits) {
        ++packet_id;
        if (bits & 1) {
          packet_nacks.push_back(PacketNack{frame_id, packet_id});
        }
        bits >>= 1;
      }
    }
  }

  // Parse the optional CST2 feedback (frame-level ACKs).
  if (static_cast<int>(in.size()) < kRtcpFeedbackAckHeaderSize ||
      ConsumeField<uint32_t>(in) != kRtcpCst2IdentifierWord) {
    // Optional CST2 extended feedback is not present. For backwards-
    // compatibility reasons, do not consider any extra "garbage" in the packet
    // that doesn't match 'CST2' as corrupted input.
    return true;
  }
  // Skip over the "Feedback Count" field. It's currently unused, though it
  // might be useful for event tracing later...
  in.remove_prefix(sizeof(uint8_t));
  const int ack_bitvector_octet_count = ConsumeField<uint8_t>(in);
  if (static_cast<int>(in.size()) < ack_bitvector_octet_count) {
    return false;
  }
  // Translate each set bit in the bit vector into a FrameId. See the
  // explanation of this wire format in rtp_defines.h for where the "plus two"
  // comes from.
  FrameId starting_frame_id = feedback_frame_id + 2;
  for (int i = 0; i < ack_bitvector_octet_count; ++i) {
    uint8_t bits = ConsumeField<uint8_t>(in);
    FrameId frame_id = starting_frame_id;
    while (bits) {
      if (bits & 1) {
        received_frames->push_back(frame_id);
      }
      ++frame_id;
      bits >>= 1;
    }
    constexpr int kBitsPerOctet = 8;
    starting_frame_id += kBitsPerOctet;
  }

  return true;
}

bool CompoundRtcpParser::ParseExtendedReports(
    ByteView in,
    Clock::time_point& receiver_reference_time) {
  if (static_cast<int>(in.size()) < kRtcpExtendedReportHeaderSize) {
    return false;
  }
  if (ConsumeField<uint32_t>(in) != session_->receiver_ssrc()) {
    return true;  // Ignore report from unknown receiver.
  }

  while (!in.empty()) {
    // All extended report types have the same 4-byte subheader.
    if (static_cast<int>(in.size()) < kRtcpExtendedReportBlockHeaderSize) {
      return false;
    }
    const uint8_t block_type = ConsumeField<uint8_t>(in);
    in.remove_prefix(sizeof(uint8_t));  // Skip the "reserved" byte.
    const int block_data_size =
        static_cast<int>(ConsumeField<uint16_t>(in)) * 4;
    if (static_cast<int>(in.size()) < block_data_size) {
      return false;
    }
    if (block_type == kRtcpReceiverReferenceTimeReportBlockType) {
      if (block_data_size != sizeof(uint64_t)) {
        return false;  // Length field must always be 2 words.
      }
      receiver_reference_time = session_->ntp_converter().ToLocalTime(
          ReadBigEndian<uint64_t>(in.data()));
    } else {
      // Ignore any other type of extended report.
    }
    in.remove_prefix(block_data_size);
  }

  return true;
}

bool CompoundRtcpParser::ParsePictureLossIndicator(
    ByteView in,
    bool& picture_loss_indicator) {
  if (static_cast<int>(in.size()) < kRtcpPictureLossIndicatorHeaderSize) {
    return false;
  }
  // Only set the flag if the PLI is from the Receiver and to this Sender.
  if (ConsumeField<uint32_t>(in) == session_->receiver_ssrc() &&
      ConsumeField<uint32_t>(in) == session_->sender_ssrc()) {
    picture_loss_indicator = true;
  }
  return true;
}

CompoundRtcpParser::Client::Client() = default;
CompoundRtcpParser::Client::~Client() = default;
void CompoundRtcpParser::Client::OnReceiverReferenceTimeAdvanced(
    Clock::time_point reference_time) {}
void CompoundRtcpParser::Client::OnReceiverReport(
    const RtcpReportBlock& receiver_report) {}
void CompoundRtcpParser::Client::OnCastReceiverFrameLogMessages(
    std::vector<RtcpReceiverFrameLogMessage> messages) {}
void CompoundRtcpParser::Client::OnReceiverIndicatesPictureLoss() {}
void CompoundRtcpParser::Client::OnReceiverCheckpoint(
    FrameId frame_id,
    milliseconds playout_delay) {}
void CompoundRtcpParser::Client::OnReceiverHasFrames(
    std::vector<FrameId> acks) {}
void CompoundRtcpParser::Client::OnReceiverIsMissingPackets(
    std::vector<PacketNack> nacks) {}

}  // namespace cast
}  // namespace openscreen
