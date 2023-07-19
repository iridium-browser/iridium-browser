// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/sender.h"

#include <algorithm>
#include <chrono>
#include <ratio>

#include "cast/streaming/session_config.h"
#include "platform/base/trivial_clock_traits.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"
#include "util/std_util.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace cast {

using clock_operators::operator<<;

Sender::Sender(Environment* environment,
               SenderPacketRouter* packet_router,
               SessionConfig config,
               RtpPayloadType rtp_payload_type)
    : config_(config),
      packet_router_(packet_router),
      rtcp_session_(config.sender_ssrc,
                    config.receiver_ssrc,
                    environment->now()),
      rtcp_parser_(&rtcp_session_, this),
      sender_report_builder_(&rtcp_session_),
      rtp_packetizer_(rtp_payload_type,
                      config.sender_ssrc,
                      packet_router_->max_packet_size()),
      rtp_timebase_(config.rtp_timebase),
      crypto_(config.aes_secret_key, config.aes_iv_mask),
      target_playout_delay_(config.target_playout_delay) {
  OSP_DCHECK(packet_router_);
  OSP_DCHECK_NE(rtcp_session_.sender_ssrc(), rtcp_session_.receiver_ssrc());
  OSP_DCHECK_GT(rtp_timebase_, 0);
  OSP_DCHECK(target_playout_delay_ > milliseconds::zero());

  pending_sender_report_.reference_time = SenderPacketRouter::kNever;

  packet_router_->OnSenderCreated(rtcp_session_.receiver_ssrc(), this);
}

Sender::~Sender() {
  packet_router_->OnSenderDestroyed(rtcp_session_.receiver_ssrc());
}

void Sender::SetObserver(Sender::Observer* observer) {
  observer_ = observer;
}

int Sender::GetInFlightFrameCount() const {
  return num_frames_in_flight_;
}

Clock::duration Sender::GetInFlightMediaDuration(
    RtpTimeTicks next_frame_rtp_timestamp) const {
  if (num_frames_in_flight_ == 0) {
    return Clock::duration::zero();  // No frames are currently in-flight.
  }

  const PendingFrameSlot& oldest_slot = *get_slot_for(checkpoint_frame_id_ + 1);
  // Note: The oldest slot's frame cannot have been canceled because the
  // protocol does not allow ACK'ing this particular frame without also moving
  // the checkpoint forward. See "CST2 feedback" discussion in rtp_defines.h.
  OSP_DCHECK(oldest_slot.is_active_for_frame(checkpoint_frame_id_ + 1));

  return (next_frame_rtp_timestamp - oldest_slot.frame->rtp_timestamp)
      .ToDuration<Clock::duration>(rtp_timebase_);
}

Clock::duration Sender::GetMaxInFlightMediaDuration() const {
  // Assumption: The total amount of allowed in-flight media should equal the
  // half of the playout delay window, plus the amount of time it takes to
  // receive an ACK from the Receiver.
  //
  // Why half of the playout delay window? It's assumed here that capture and
  // media encoding, which occur before EnqueueFrame() is called, are executing
  // within the first half of the playout delay window. This leaves the second
  // half for executing all network transmits/re-transmits, plus decoding and
  // play-out at the Receiver.
  return (target_playout_delay_ / 2) + (round_trip_time_ / 2);
}

bool Sender::NeedsKeyFrame() const {
  return last_enqueued_key_frame_id_ <= picture_lost_at_frame_id_;
}

FrameId Sender::GetNextFrameId() const {
  return last_enqueued_frame_id_ + 1;
}

Clock::duration Sender::GetCurrentRoundTripTime() const {
  return round_trip_time_;
}

Sender::EnqueueFrameResult Sender::EnqueueFrame(const EncodedFrame& frame) {
  // Assume the fields of the |frame| have all been set correctly, with
  // monotonically increasing timestamps and a valid pointer to the data.
  OSP_DCHECK_EQ(frame.frame_id, GetNextFrameId());
  OSP_DCHECK_GE(frame.referenced_frame_id, FrameId::first());
  if (frame.frame_id != FrameId::first()) {
    OSP_DCHECK_GT(frame.rtp_timestamp, pending_sender_report_.rtp_timestamp);
    OSP_DCHECK_GT(frame.reference_time, pending_sender_report_.reference_time);
  }
  OSP_DCHECK(frame.data.data());

  // Check whether enqueuing the frame would exceed the design limit for the
  // span of FrameIds. Even if |num_frames_in_flight_| is less than
  // kMaxUnackedFrames, it's the span of FrameIds that is restricted.
  if ((frame.frame_id - checkpoint_frame_id_) > kMaxUnackedFrames) {
    return REACHED_ID_SPAN_LIMIT;
  }

  // Check whether enqueuing the frame would exceed the current maximum media
  // duration limit.
  if (GetInFlightMediaDuration(frame.rtp_timestamp) >
      GetMaxInFlightMediaDuration()) {
    return MAX_DURATION_IN_FLIGHT;
  }

  // Encrypt the frame and initialize the slot tracking its sending.
  PendingFrameSlot* const slot = get_slot_for(frame.frame_id);
  OSP_DCHECK(!slot->frame);
  slot->frame = crypto_.Encrypt(frame);
  const int packet_count = rtp_packetizer_.ComputeNumberOfPackets(*slot->frame);
  if (packet_count <= 0) {
    slot->frame.reset();
    return PAYLOAD_TOO_LARGE;
  }
  slot->send_flags.Resize(packet_count, YetAnotherBitVector::SET);
  slot->packet_sent_times.assign(packet_count, SenderPacketRouter::kNever);

  // Officially record the "enqueue."
  ++num_frames_in_flight_;
  last_enqueued_frame_id_ = slot->frame->frame_id;
  OSP_DCHECK_LE(num_frames_in_flight_,
                last_enqueued_frame_id_ - checkpoint_frame_id_);
  if (slot->frame->dependency == EncodedFrame::Dependency::kKeyFrame) {
    last_enqueued_key_frame_id_ = slot->frame->frame_id;
  }

  // Update the target playout delay, if necessary.
  if (slot->frame->new_playout_delay > milliseconds::zero()) {
    target_playout_delay_ = slot->frame->new_playout_delay;
    playout_delay_change_at_frame_id_ = slot->frame->frame_id;
  }

  // Update the lip-sync information for the next Sender Report.
  pending_sender_report_.reference_time = slot->frame->reference_time;
  pending_sender_report_.rtp_timestamp = slot->frame->rtp_timestamp;

  // If the round trip time hasn't been computed yet, immediately send a RTCP
  // packet (i.e., before the RTP packets are sent). The RTCP packet will
  // provide a Sender Report which contains the required lip-sync information
  // the Receiver needs for timing the media playout.
  //
  // Detail: Working backwards, if the round trip time is not known, then this
  // Sender has never processed a Receiver Report. Thus, the Receiver has never
  // provided a Receiver Report, which it can only do after having processed a
  // Sender Report from this Sender. Thus, this Sender really needs to send
  // that, right now!
  if (round_trip_time_ == Clock::duration::zero()) {
    packet_router_->RequestRtcpSend(rtcp_session_.receiver_ssrc());
  }

  // Re-activate RTP sending if it was suspended.
  packet_router_->RequestRtpSend(rtcp_session_.receiver_ssrc());

  return OK;
}

void Sender::CancelInFlightData() {
  TRACE_SCOPED1(TraceCategory::kSender, "CancelInFlightData",
                "frames_in_flight",
                std::to_string(last_enqueued_frame_id_ - checkpoint_frame_id_));

  while (checkpoint_frame_id_ < last_enqueued_frame_id_) {
    ++checkpoint_frame_id_;
    CancelPendingFrame(checkpoint_frame_id_);
  }
}

void Sender::OnReceivedRtcpPacket(Clock::time_point arrival_time,
                                  absl::Span<const uint8_t> packet) {
  rtcp_packet_arrival_time_ = arrival_time;
  // This call to Parse() invoke zero or more of the OnReceiverXYZ() methods in
  // the current call stack:
  if (rtcp_parser_.Parse(packet, last_enqueued_frame_id_)) {
    packet_router_->OnRtcpReceived(arrival_time, round_trip_time_);
  }
}

absl::Span<uint8_t> Sender::GetRtcpPacketForImmediateSend(
    Clock::time_point send_time,
    absl::Span<uint8_t> buffer) {
  if (pending_sender_report_.reference_time == SenderPacketRouter::kNever) {
    // Cannot send a report if one is not available (i.e., a frame has never
    // been enqueued).
    return buffer.subspan(0, 0);
  }

  // The Sender Report to be sent is a snapshot of the "pending Sender Report,"
  // but with its timestamp fields modified. First, the reference time is set to
  // the RTCP packet's send time. Then, the corresponding RTP timestamp is
  // translated to match (for lip-sync).
  RtcpSenderReport sender_report = pending_sender_report_;
  sender_report.reference_time = send_time;
  sender_report.rtp_timestamp += RtpTimeDelta::FromDuration(
      sender_report.reference_time - pending_sender_report_.reference_time,
      rtp_timebase_);

  return sender_report_builder_.BuildPacket(sender_report, buffer).first;
}

absl::Span<uint8_t> Sender::GetRtpPacketForImmediateSend(
    Clock::time_point send_time,
    absl::Span<uint8_t> buffer) {
  ChosenPacket chosen = ChooseNextRtpPacketNeedingSend();

  // If no packets need sending (i.e., all packets have been sent at least once
  // and do not need to be re-sent yet), check whether a Kickstart packet should
  // be sent. It's possible that there has been complete packet loss of some
  // frames, and the Receiver may not be aware of the existence of the latest
  // frame(s). Kickstarting is the only way the Receiver can discover the newer
  // frames it doesn't know about.
  if (!chosen) {
    const ChosenPacketAndWhen kickstart = ChooseKickstartPacket();
    if (kickstart.when > send_time) {
      // Nothing to send, so return "empty" signal to the packet router. The
      // packet router will suspend RTP sending until this Sender explicitly
      // resumes it.
      return buffer.subspan(0, 0);
    }
    chosen = kickstart;
    OSP_DCHECK(chosen);
  }

  const absl::Span<uint8_t> result = rtp_packetizer_.GeneratePacket(
      *chosen.slot->frame, chosen.packet_id, buffer);
  chosen.slot->send_flags.Clear(chosen.packet_id);
  chosen.slot->packet_sent_times[chosen.packet_id] = send_time;

  ++pending_sender_report_.send_packet_count;
  // According to RFC3550, the octet count does not include the RTP header. The
  // following is just a good approximation, however, because the header size
  // will very infrequently be 4 bytes greater (see
  // RtpPacketizer::kAdaptiveLatencyHeaderSize). No known Cast Streaming
  // Receiver implementations use this for anything, and so this should be fine.
  const int approximate_octet_count =
      static_cast<int>(result.size()) - RtpPacketizer::kBaseRtpHeaderSize;
  OSP_DCHECK_GE(approximate_octet_count, 0);
  pending_sender_report_.send_octet_count += approximate_octet_count;

  return result;
}

Clock::time_point Sender::GetRtpResumeTime() {
  if (ChooseNextRtpPacketNeedingSend()) {
    return Alarm::kImmediately;
  }
  return ChooseKickstartPacket().when;
}

void Sender::OnReceiverReferenceTimeAdvanced(Clock::time_point reference_time) {
  // Not used.
}

void Sender::OnReceiverReport(const RtcpReportBlock& receiver_report) {
  OSP_DCHECK_NE(rtcp_packet_arrival_time_, SenderPacketRouter::kNever);

  const Clock::duration total_delay =
      rtcp_packet_arrival_time_ -
      sender_report_builder_.GetRecentReportTime(
          receiver_report.last_status_report_id, rtcp_packet_arrival_time_);
  const auto non_network_delay =
      Clock::to_duration(receiver_report.delay_since_last_report);

  // Round trip time measurement: This is the time elapsed since the Sender
  // Report was sent, minus the time the Receiver did other stuff before sending
  // the Receiver Report back.
  //
  // If the round trip time seems to be less than or equal to zero, assume clock
  // imprecision by one or both peers caused a bad value to be calculated. The
  // true value is likely very close to zero (i.e., this is ideal network
  // behavior); and so just represent this as 75 µs, an optimistic
  // wired-Ethernet LAN ping time.
  constexpr auto kNearZeroRoundTripTime = Clock::to_duration(microseconds(75));
  static_assert(kNearZeroRoundTripTime > Clock::duration::zero(),
                "More precision in Clock::duration needed!");
  const Clock::duration measurement =
      std::max(total_delay - non_network_delay, kNearZeroRoundTripTime);

  // Validate the measurement by using the current target playout delay as a
  // "reasonable upper-bound." It's certainly possible that the actual network
  // round-trip time could exceed the target playout delay, but that would mean
  // the current network performance is totally inadequate for streaming anyway.
  if (measurement > target_playout_delay_) {
    OSP_LOG_WARN << "Invalidating a round-trip time measurement ("
                 << measurement
                 << ") since it exceeds the current target playout delay ("
                 << target_playout_delay_ << ").";
    return;
  }

  // Measurements will typically have high variance. Use a simple smoothing
  // filter to track a short-term average that changes less drastically.
  if (round_trip_time_ == Clock::duration::zero()) {
    round_trip_time_ = measurement;
  } else {
    // Arbitrary constant, to provide 1/8 weight to the new measurement, and 7/8
    // weight to the old estimate, which seems to work well for de-noising the
    // estimate.
    constexpr int kInertia = 7;
    round_trip_time_ =
        (kInertia * round_trip_time_ + measurement) / (kInertia + 1);
  }
  TRACE_SCOPED1(TraceCategory::kSender, "UpdatedRoundTripTime",
                "round_trip_time", ToString(round_trip_time_));
}

void Sender::OnReceiverIndicatesPictureLoss() {
  TRACE_SCOPED(TraceCategory::kSender, "OnReceiverIndicatesPictureLoss");
  // The Receiver will continue the PLI notifications until it has received a
  // key frame. Thus, if a key frame is already in-flight, don't make a state
  // change that would cause this Sender to force another expensive key frame.
  if (checkpoint_frame_id_ < last_enqueued_key_frame_id_) {
    return;
  }

  picture_lost_at_frame_id_ = checkpoint_frame_id_;

  if (observer_) {
    observer_->OnPictureLost();
  }

  // Note: It may seem that all pending frames should be canceled until
  // EnqueueFrame() is called with a key frame. However:
  //
  //   1. The Receiver should still be the main authority on what frames/packets
  //      are being ACK'ed and NACK'ed.
  //
  //   2. It may be desirable for the Receiver to be "limping along" in the
  //      meantime. For example, video may be corrupted but mostly watchable,
  //      and so it's best for the Sender to continue sending the non-key frames
  //      until the Receiver indicates otherwise.
}

void Sender::OnReceiverCheckpoint(FrameId frame_id,
                                  milliseconds playout_delay) {
  TRACE_SCOPED2(TraceCategory::kSender, "OnReceiverCheckpoint", "frame_id",
                frame_id.ToString(), "playout_delay", ToString(playout_delay));
  if (frame_id > last_enqueued_frame_id_) {
    OSP_LOG_ERROR
        << "Ignoring checkpoint for " << latest_expected_frame_id_
        << " because this Sender could not have sent any frames after "
        << last_enqueued_frame_id_ << '.';
    return;
  }
  // CompoundRtcpParser should guarantee this:
  OSP_DCHECK(playout_delay >= milliseconds::zero());

  while (checkpoint_frame_id_ < frame_id) {
    ++checkpoint_frame_id_;
    CancelPendingFrame(checkpoint_frame_id_);
  }
  latest_expected_frame_id_ = std::max(latest_expected_frame_id_, frame_id);

  if (playout_delay != target_playout_delay_ &&
      frame_id >= playout_delay_change_at_frame_id_) {
    OSP_LOG_WARN << "Sender's target playout delay (" << target_playout_delay_
                 << ") disagrees with the Receiver's (" << playout_delay << ")";
  }
}

void Sender::OnReceiverHasFrames(std::vector<FrameId> acks) {
  OSP_DCHECK(!acks.empty() && AreElementsSortedAndUnique(acks));
  TRACE_SCOPED1(TraceCategory::kSender, "OnReceiverHasFrames", "frame_ids",
                Join(acks));

  if (acks.back() > last_enqueued_frame_id_) {
    OSP_LOG_ERROR << "Ignoring individual frame ACKs: ACKing frame "
                  << latest_expected_frame_id_
                  << " is invalid because this Sender could not have sent any "
                     "frames after "
                  << last_enqueued_frame_id_ << '.';
    return;
  }

  for (FrameId id : acks) {
    CancelPendingFrame(id);
  }
  latest_expected_frame_id_ = std::max(latest_expected_frame_id_, acks.back());
}

void Sender::OnReceiverIsMissingPackets(std::vector<PacketNack> nacks) {
  TRACE_SCOPED1(TraceCategory::kSender, "OnReceiverIsMissingPackets",
                "number_of_packets", std::to_string(nacks.size()));
  OSP_DCHECK(!nacks.empty() && AreElementsSortedAndUnique(nacks));
  OSP_DCHECK_NE(rtcp_packet_arrival_time_, SenderPacketRouter::kNever);

  // This is a point-in-time threshold that indicates whether each NACK will
  // trigger a packet retransmit. The threshold is based on the network round
  // trip time because a Receiver's NACK may have been issued while the needed
  // packet was in-flight from the Sender. In such cases, the Receiver's NACK is
  // likely stale and this Sender should not redundantly re-transmit the packet
  // again.
  const Clock::time_point too_recent_a_send_time =
      rtcp_packet_arrival_time_ - round_trip_time_;

  // Iterate over all the NACKs...
  bool need_to_send = false;
  for (auto nack_it = nacks.begin(); nack_it != nacks.end();) {
    // Find the slot associated with the NACK's frame ID.
    const FrameId frame_id = nack_it->frame_id;
    PendingFrameSlot* slot = nullptr;
    if (frame_id <= last_enqueued_frame_id_) {
      PendingFrameSlot* const candidate_slot = get_slot_for(frame_id);
      if (candidate_slot->is_active_for_frame(frame_id)) {
        slot = candidate_slot;
      }
    }

    // If no slot was found (i.e., the NACK is invalid) for the frame, skip-over
    // all other NACKs for the same frame. While it seems to be a bug that the
    // Receiver would attempt to NACK a frame that does not yet exist, this can
    // happen in rare cases where RTCP packets arrive out-of-order (i.e., the
    // network shuffled them).
    if (!slot) {
      TRACE_SCOPED(TraceCategory::kSender, "MissingNackSlot");
      for (++nack_it; nack_it != nacks.end() && nack_it->frame_id == frame_id;
           ++nack_it) {
      }
      continue;
    }

    latest_expected_frame_id_ = std::max(latest_expected_frame_id_, frame_id);

    const auto HandleIndividualNack = [&](FramePacketId packet_id) {
      if (slot->packet_sent_times[packet_id] <= too_recent_a_send_time) {
        slot->send_flags.Set(packet_id);
        need_to_send = true;
      }
    };
    const FramePacketId range_end = slot->packet_sent_times.size();
    if (nack_it->packet_id == kAllPacketsLost) {
      for (FramePacketId packet_id = 0; packet_id < range_end; ++packet_id) {
        HandleIndividualNack(packet_id);
      }
      ++nack_it;
    } else {
      do {
        if (nack_it->packet_id < range_end) {
          HandleIndividualNack(nack_it->packet_id);
        } else {
          OSP_LOG_WARN
              << "Ignoring NACK for packet that doesn't exist in frame "
              << frame_id << ": " << static_cast<int>(nack_it->packet_id);
        }
        ++nack_it;
      } while (nack_it != nacks.end() && nack_it->frame_id == frame_id);
    }
  }

  if (need_to_send) {
    packet_router_->RequestRtpSend(rtcp_session_.receiver_ssrc());
  }
}

Sender::ChosenPacket Sender::ChooseNextRtpPacketNeedingSend() {
  // Find the oldest packet needing to be sent (or re-sent).
  for (FrameId frame_id = checkpoint_frame_id_ + 1;
       frame_id <= last_enqueued_frame_id_; ++frame_id) {
    PendingFrameSlot* const slot = get_slot_for(frame_id);
    if (!slot->is_active_for_frame(frame_id)) {
      continue;  // Frame was canceled. None of its packets need to be sent.
    }
    const FramePacketId packet_id = slot->send_flags.FindFirstSet();
    if (packet_id < slot->send_flags.size()) {
      return {slot, packet_id};
    }
  }

  return {};  // Nothing needs to be sent.
}

Sender::ChosenPacketAndWhen Sender::ChooseKickstartPacket() {
  if (latest_expected_frame_id_ >= last_enqueued_frame_id_) {
    // Since the Receiver must know about all of the frames currently queued, no
    // Kickstart packet is necessary.
    return {};
  }

  // The Kickstart packet is always in the last-enqueued frame, so that the
  // Receiver will know about every frame the Sender has. However, which packet
  // should be chosen? Any would do, since all packets contain the frame's total
  // packet count. For historical reasons, all sender implementations have
  // always just sent the last packet; and so that tradition is continued here.
  ChosenPacketAndWhen chosen;
  chosen.slot = get_slot_for(last_enqueued_frame_id_);
  // Note: This frame cannot have been canceled since
  // |latest_expected_frame_id_| hasn't yet reached this point.
  OSP_DCHECK(chosen.slot->is_active_for_frame(last_enqueued_frame_id_));
  chosen.packet_id = chosen.slot->send_flags.size() - 1;

  const Clock::time_point time_last_sent =
      chosen.slot->packet_sent_times[chosen.packet_id];
  // Sanity-check: This method should not be called to choose a packet while
  // there are still unsent packets.
  OSP_DCHECK_NE(time_last_sent, SenderPacketRouter::kNever);

  // The desired Kickstart interval is a fraction of the total
  // |target_playout_delay_|. The reason for the specific ratio here is based on
  // lost knowledge (from legacy implementations); but it makes sense (i.e., to
  // be a good "network citizen") to be less aggressive for larger playout delay
  // windows, and more aggressive for shorter ones to avoid too-late packet
  // arrivals.
  using kWaitFraction = std::ratio<1, 20>;
  const Clock::duration desired_kickstart_interval =
      Clock::to_duration(target_playout_delay_) * kWaitFraction::num /
      kWaitFraction::den;
  // The actual interval used is increased, if current network performance
  // warrants waiting longer. Don't send a Kickstart packet until no NACKs
  // have been received for two network round-trip periods.
  constexpr int kLowerBoundRoundTrips = 2;
  const Clock::duration kickstart_interval = std::max(
      desired_kickstart_interval, round_trip_time_ * kLowerBoundRoundTrips);
  chosen.when = time_last_sent + kickstart_interval;

  return chosen;
}

void Sender::CancelPendingFrame(FrameId frame_id) {
  TRACE_SCOPED1(TraceCategory::kSender, "CancelPendingFrame", "frame_id",
                frame_id.ToString());
  PendingFrameSlot* const slot = get_slot_for(frame_id);
  if (!slot->is_active_for_frame(frame_id)) {
    return;  // Frame was already canceled.
  }

  packet_router_->OnPayloadReceived(
      slot->frame->data.size(), rtcp_packet_arrival_time_, round_trip_time_);

  slot->frame.reset();
  OSP_DCHECK_GT(num_frames_in_flight_, 0);
  --num_frames_in_flight_;
  if (observer_) {
    observer_->OnFrameCanceled(frame_id);
  }
}

void Sender::Observer::OnFrameCanceled(FrameId frame_id) {}
void Sender::Observer::OnPictureLost() {}
Sender::Observer::~Observer() = default;

Sender::PendingFrameSlot::PendingFrameSlot() = default;
Sender::PendingFrameSlot::~PendingFrameSlot() = default;

}  // namespace cast
}  // namespace openscreen
