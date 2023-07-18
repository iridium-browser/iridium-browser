// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_SENDER_H_
#define CAST_STREAMING_SENDER_H_

#include <stdint.h>

#include <array>
#include <chrono>
#include <vector>

#include "absl/types/span.h"
#include "cast/streaming/compound_rtcp_parser.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/frame_crypto.h"
#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtp_defines.h"
#include "cast/streaming/rtp_packetizer.h"
#include "cast/streaming/rtp_time.h"
#include "cast/streaming/sender_packet_router.h"
#include "cast/streaming/sender_report_builder.h"
#include "cast/streaming/session_config.h"
#include "platform/api/time.h"
#include "util/yet_another_bit_vector.h"

namespace openscreen {
namespace cast {

class Environment;

// The Cast Streaming Sender, a peer corresponding to some Cast Streaming
// Receiver at the other end of a network link. See class level comments for
// Receiver for a high-level overview.
//
// The Sender is the peer responsible for enqueuing EncodedFrames for streaming,
// guaranteeing their delivery to a Receiver, and handling feedback events from
// a Receiver. Some feedback events are used for managing the Sender's internal
// queue of in-flight frames, requesting network packet re-transmits, etc.;
// while others are exposed via the Sender's public interface. For example,
// sometimes the Receiver signals that it needs a a key frame to resolve a
// picture loss condition, and the modules upstream of the Sender (e.g., where
// encoding happens) should call NeedsKeyFrame() to check for, and handle that.
//
// There are usually one or two Senders in a streaming session, one for audio
// and one for video. Both senders work with the same SenderPacketRouter
// instance to schedule their transmission of packets, and provide the necessary
// metrics for estimating bandwidth utilization and availability.
//
// It is the responsibility of upstream code modules to handle congestion
// control. With respect to this Sender, that means the media encoding bit rate
// should be throttled based on network bandwidth availability. This Sender does
// not do any throttling, only flow-control. In other words, this Sender can
// only manage its in-flight queue of frames, and if that queue grows too large,
// it will eventually reject further enqueuing.
//
// General usage: A client should check the in-flight media duration frequently
// to decide when to pause encoding, to avoid wasting system resources on
// encoding frames that will likely be rejected by the Sender. The client should
// also frequently call NeedsKeyFrame() and, when this returns true, direct its
// encoder to produce a key frame soon. Finally, when using EnqueueFrame(), an
// EncodedFrame struct should be prepared with its frame_id field set to
// whatever GetNextFrameId() returns. Please see method comments for
// more-detailed usage info.
class Sender final : public SenderPacketRouter::Sender,
                     public CompoundRtcpParser::Client {
 public:
  // Interface for receiving notifications about events of possible interest.
  // Handling each of these is optional, but some may be mandatory for certain
  // applications (see method comments below).
  class Observer {
   public:
    // Called when a frame was canceled. "Canceled" means that the Receiver has
    // either acknowledged successful receipt of the frame or has decided to
    // skip over it. Note: Frame cancellations may occur out-of-order.
    virtual void OnFrameCanceled(FrameId frame_id);

    // Called when a Receiver begins reporting picture loss, and there is no key
    // frame currently enqueued in the Sender. The application should enqueue a
    // key frame as soon as possible. Note: An application that pauses frame
    // sending (e.g., screen mirroring when the screen is not changing) should
    // use this notification to send an out-of-band "refresh frame," encoded as
    // a key frame.
    virtual void OnPictureLost();

   protected:
    virtual ~Observer();
  };

  // Result codes for EnqueueFrame().
  enum EnqueueFrameResult {
    // The frame has been queued for sending.
    OK,

    // The frame's payload was too large. This is typically triggered when
    // submitting a payload of several dozen megabytes or more. This result code
    // likely indicates some kind of upstream bug.
    PAYLOAD_TOO_LARGE,

    // The span of FrameIds is too large. Cast Streaming's protocol design
    // imposes a limit in the maximum difference between the highest-valued
    // in-flight FrameId and the least-valued one.
    REACHED_ID_SPAN_LIMIT,

    // Too-large a media duration is in-flight. Enqueuing another frame would
    // automatically cause late play-out at the Receiver.
    MAX_DURATION_IN_FLIGHT,
  };

  // Constructs a Sender that attaches to the given |environment|-provided
  // resources and |packet_router|. The |config| contains the settings that were
  // agreed-upon by both sides from the OFFER/ANSWER exchange (i.e., the part of
  // the overall end-to-end connection process that occurs before Cast Streaming
  // is started). The |rtp_payload_type| does not affect the behavior of this
  // Sender. It is simply passed along to a Receiver in the RTP packet stream.
  Sender(Environment* environment,
         SenderPacketRouter* packet_router,
         SessionConfig config,
         RtpPayloadType rtp_payload_type);

  ~Sender() final;

  const SessionConfig& config() const { return config_; }
  Ssrc ssrc() const { return rtcp_session_.sender_ssrc(); }
  int rtp_timebase() const { return rtp_timebase_; }

  // Sets an observer for receiving notifications. Call with nullptr to stop
  // observing.
  void SetObserver(Observer* observer);

  // Returns the number of frames currently in-flight. This is only meant to be
  // informative. Clients should use GetInFlightMediaDuration() to make
  // throttling decisions.
  int GetInFlightFrameCount() const;

  // Returns the total media duration of the frames currently in-flight,
  // assuming the next not-yet-enqueued frame will have the given RTP timestamp.
  // For a better user experience, the result should be compared to
  // GetMaxInFlightMediaDuration(), and media encoding should be throttled down
  // before additional EnqueueFrame() calls would cause this to reach the
  // current maximum limit.
  Clock::duration GetInFlightMediaDuration(
      RtpTimeTicks next_frame_rtp_timestamp) const;

  // Return the maximum acceptable in-flight media duration, given the current
  // target playout delay setting and end-to-end network/system conditions.
  Clock::duration GetMaxInFlightMediaDuration() const;

  // Returns true if the Receiver requires a key frame. Note that this will
  // return true until a key frame is accepted by EnqueueFrame(). Thus, when
  // encoding is pipelined, care should be taken to instruct the encoder to
  // produce just ONE forced key frame.
  bool NeedsKeyFrame() const;

  // Returns the next FrameId, the one after the frame enqueued by the last call
  // to EnqueueFrame(). Note that the next call to EnqueueFrame() assumes this
  // frame ID be used.
  FrameId GetNextFrameId() const;

  // Get the current round trip time, defined as the total time between when the
  // sender report is sent and the receiver report is received. This value is
  // updated with each receiver report using a weighted moving average of 1/8
  // for the new value and 7/8 for the previous value. Will be set to
  // Clock::duration::zero() if no reports have been received yet.
  Clock::duration GetCurrentRoundTripTime() const;

  // Enqueues the given |frame| for sending as soon as possible. Returns OK if
  // the frame is accepted, and some time later Observer::OnFrameCanceled() will
  // be called once it is no longer in-flight.
  //
  // All fields of the |frame| must be set to valid values: the |frame_id| must
  // be the same as GetNextFrameId(); both the |rtp_timestamp| and
  // |reference_time| fields must be monotonically increasing relative to the
  // prior frame; and the frame's |data| pointer must be set.
  [[nodiscard]] EnqueueFrameResult EnqueueFrame(const EncodedFrame& frame);

  // Causes all pending operations to discard data when they are processed
  // later.
  void CancelInFlightData();

 private:
  // Tracking/Storage for frames that are ready-to-send, and until they are
  // fully received at the other end.
  struct PendingFrameSlot {
    // The frame to send, or nullopt if this slot is not in use.
    absl::optional<EncryptedFrame> frame;

    // Represents which packets need to be sent. Elements are indexed by
    // FramePacketId. A set bit means a packet needs to be sent (or re-sent).
    YetAnotherBitVector send_flags;

    // The time when each of the packets was last sent, or
    // |SenderPacketRouter::kNever| if the packet has not been sent yet.
    // Elements are indexed by FramePacketId. This is used to avoid
    // re-transmitting any given packet too frequently.
    std::vector<Clock::time_point> packet_sent_times;

    PendingFrameSlot();
    ~PendingFrameSlot();

    bool is_active_for_frame(FrameId frame_id) const {
      return frame && frame->frame_id == frame_id;
    }
  };

  // Return value from the ChooseXYZ() helper methods.
  struct ChosenPacket {
    PendingFrameSlot* slot = nullptr;
    FramePacketId packet_id{};

    explicit operator bool() const { return !!slot; }
  };

  // An extension of ChosenPacket that also includes the point-in-time when the
  // packet should be sent.
  struct ChosenPacketAndWhen : public ChosenPacket {
    Clock::time_point when = SenderPacketRouter::kNever;
  };

  // SenderPacketRouter::Sender implementation.
  void OnReceivedRtcpPacket(Clock::time_point arrival_time,
                            absl::Span<const uint8_t> packet) final;
  absl::Span<uint8_t> GetRtcpPacketForImmediateSend(
      Clock::time_point send_time,
      absl::Span<uint8_t> buffer) final;
  absl::Span<uint8_t> GetRtpPacketForImmediateSend(
      Clock::time_point send_time,
      absl::Span<uint8_t> buffer) final;
  Clock::time_point GetRtpResumeTime() final;

  // CompoundRtcpParser::Client implementation.
  void OnReceiverReferenceTimeAdvanced(Clock::time_point reference_time) final;
  void OnReceiverReport(const RtcpReportBlock& receiver_report) final;
  void OnReceiverIndicatesPictureLoss() final;
  void OnReceiverCheckpoint(FrameId frame_id,
                            std::chrono::milliseconds playout_delay) final;
  void OnReceiverHasFrames(std::vector<FrameId> acks) final;
  void OnReceiverIsMissingPackets(std::vector<PacketNack> nacks) final;

  // Helper to choose which packet to send, from those that have been flagged as
  // "need to send." Returns a "false" result if nothing needs to be sent.
  ChosenPacket ChooseNextRtpPacketNeedingSend();

  // Helper that returns the packet that should be used to kick-start the
  // Receiver, and the time at which the packet should be sent. Returns a kNever
  // result if kick-starting is not needed.
  ChosenPacketAndWhen ChooseKickstartPacket();

  // Cancels sending (or resending) the given frame once it is known to have
  // been cancelled by the sender fully received (e.g., based on the ACK
  // feedback from the Receiver in a RTCP packet, or the receiver checkpoint
  // frame). This clears the corresponding entry in |pending_frames_| and
  // notifies the Observer. NOTE: every frame_id ends up being "cancelled" at
  // least once.
  void CancelPendingFrame(FrameId frame_id);

  // Inline helper to return the slot that would contain the tracking info for
  // the given |frame_id|.
  const PendingFrameSlot* get_slot_for(FrameId frame_id) const {
    return &pending_frames_[(frame_id - FrameId::first()) %
                            pending_frames_.size()];
  }
  PendingFrameSlot* get_slot_for(FrameId frame_id) {
    return &pending_frames_[(frame_id - FrameId::first()) %
                            pending_frames_.size()];
  }

  const SessionConfig config_;
  SenderPacketRouter* const packet_router_;
  RtcpSession rtcp_session_;
  CompoundRtcpParser rtcp_parser_;
  SenderReportBuilder sender_report_builder_;
  RtpPacketizer rtp_packetizer_;
  const int rtp_timebase_;
  FrameCrypto crypto_;

  // Ring buffer of PendingFrameSlots. The frame having FrameId x will always
  // be slotted at position x % pending_frames_.size(). Use get_slot_for() to
  // access the correct slot for a given FrameId.
  std::array<PendingFrameSlot, kMaxUnackedFrames> pending_frames_{};

  // A count of the number of frames in-flight (i.e., the number of active
  // entries in |pending_frames_|).
  int num_frames_in_flight_ = 0;

  // The ID of the last frame enqueued.
  FrameId last_enqueued_frame_id_ = FrameId::leader();

  // Indicates that all of the packets for all frames up to and including this
  // FrameId have been successfully received (or otherwise do not need to be
  // re-transmitted).
  FrameId checkpoint_frame_id_ = FrameId::leader();

  // The ID of the latest frame the Receiver seems to be aware of.
  FrameId latest_expected_frame_id_ = FrameId::leader();

  // The target playout delay for the last-enqueued frame. This is auto-updated
  // when a frame is enqueued that changes the delay.
  std::chrono::milliseconds target_playout_delay_;
  FrameId playout_delay_change_at_frame_id_ = FrameId::first();

  // The exact arrival time of the last RTCP packet.
  Clock::time_point rtcp_packet_arrival_time_ = SenderPacketRouter::kNever;

  // The near-term average round trip time. This is updated with each Sender
  // Report → Receiver Report round trip. This is initially zero, indicating the
  // round trip time has not been measured yet.
  Clock::duration round_trip_time_{0};

  // Maintain current stats in a Sender Report that is ready for sending at any
  // time. This includes up-to-date lip-sync information, and packet and byte
  // count stats.
  RtcpSenderReport pending_sender_report_;

  // These are used to determine whether a key frame needs to be sent to the
  // Receiver. When the Receiver provides a picture loss notification, the
  // current checkpoint frame ID is stored in |picture_lost_at_frame_id_|. Then,
  // while |last_enqueued_key_frame_id_| is less than or equal to
  // |picture_lost_at_frame_id_|, the Sender knows it still needs to send a key
  // frame to resolve the picture loss condition. In all other cases, the
  // Receiver is either in a good state or is in the process of receiving the
  // key frame that will make that happen.
  FrameId picture_lost_at_frame_id_ = FrameId::leader();
  FrameId last_enqueued_key_frame_id_ = FrameId::leader();

  // The current observer (optional).
  Observer* observer_ = nullptr;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_SENDER_H_
