// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_SENDER_PACKET_ROUTER_H_
#define CAST_STREAMING_SENDER_PACKET_ROUTER_H_

#include <stdint.h>

#include <chrono>
#include <memory>
#include <vector>

#include "absl/types/span.h"
#include "cast/streaming/bandwidth_estimator.h"
#include "cast/streaming/environment.h"
#include "cast/streaming/ssrc.h"
#include "platform/api/time.h"
#include "util/alarm.h"

namespace openscreen {
namespace cast {

// Manages network packet transmission for one or more Senders, directing each
// inbound packet to a specific Sender instance, pacing the transmission of
// outbound packets, and employing network bandwidth/availability monitoring and
// congestion control.
//
// Instead of just sending packets whenever they want, Senders must request
// transmission from the SenderPacketRouter. The router then calls-back to each
// Sender, in the near future, when it has allocated an available time slice for
// transmission. The Sender is allowed to decide, at that exact moment, which
// packet most needs to be sent.
//
// Pacing strategy: Packets are sent in bursts. This allows the platform
// (operating system) to collect many small packets into a short-term buffer,
// which allows for optimizations at the link layer. For example, multiple
// packets can be sent together as one larger transmission unit, and this can be
// critical for good performance over shared-medium networks (such as 802.11
// WiFi). https://en.wikipedia.org/wiki/Frame-bursting
class SenderPacketRouter : public BandwidthEstimator,
                           public Environment::PacketConsumer {
 public:
  class Sender {
   public:
    // Called to provide the Sender with what looks like a RTCP packet meant for
    // it specifically (among other Senders) to process. |arrival_time|
    // indicates when the packet arrived (i.e., when it was received from the
    // platform).
    virtual void OnReceivedRtcpPacket(Clock::time_point arrival_time,
                                      absl::Span<const uint8_t> packet) = 0;

    // Populates the given |buffer| with a RTCP/RTP packet that will be sent
    // immediately. Returns the portion of |buffer| contaning the packet, or an
    // empty Span if nothing is ready to send.
    virtual absl::Span<uint8_t> GetRtcpPacketForImmediateSend(
        Clock::time_point send_time,
        absl::Span<uint8_t> buffer) = 0;
    virtual absl::Span<uint8_t> GetRtpPacketForImmediateSend(
        Clock::time_point send_time,
        absl::Span<uint8_t> buffer) = 0;

    // Returns the point-in-time at which RTP sending should resume, or kNever
    // if it should be suspended until an explicit call to RequestRtpSend(). The
    // implementation may return a value on or before "now" to indicate an
    // immediate resume is desired.
    virtual Clock::time_point GetRtpResumeTime() = 0;

   protected:
    virtual ~Sender();
  };

  // Constructs an instance with default burst parameters appropriate for the
  // given |max_burst_bitrate|.
  explicit SenderPacketRouter(Environment* environment,
                              int max_burst_bitrate = kDefaultMaxBurstBitrate);

  // Constructs an instance with specific burst parameters. The maximum bitrate
  // will be computed based on these (and Environment::GetMaxPacketSize()).
  SenderPacketRouter(Environment* environment,
                     int max_packets_per_burst,
                     std::chrono::milliseconds burst_interval);

  ~SenderPacketRouter();

  int max_packet_size() const { return packet_buffer_size_; }
  int max_burst_bitrate() const { return max_burst_bitrate_; }

  // Called from a Sender constructor/destructor to register/deregister a Sender
  // instance that processes RTP/RTCP packets from a Receiver having the given
  // SSRC.
  void OnSenderCreated(Ssrc receiver_ssrc, Sender* client);
  void OnSenderDestroyed(Ssrc receiver_ssrc);

  // Requests an immediate send of a RTCP packet, and then RTCP sending will
  // repeat at regular intervals (see kRtcpSendInterval) until the Sender is
  // de-registered.
  void RequestRtcpSend(Ssrc receiver_ssrc);

  // Requests an immediate send of a RTP packet. RTP sending will continue until
  // the Sender stops providing packet data.
  //
  // See also: Sender::GetRtpResumeTime().
  void RequestRtpSend(Ssrc receiver_ssrc);

  // A reasonable default maximum bitrate for bursting. Congestion control
  // should always be employed to limit the Senders' sustained/average outbound
  // data volume for "fair" use of the network.
  static constexpr int kDefaultMaxBurstBitrate = 24 << 20;  // 24 megabits/sec

  // The minimum amount of time between burst-sends. The methodology by which
  // this value was determined is lost knowledge, but is likely the result of
  // experimentation with various network and operating system configurations.
  // This value came from the original Chrome Cast Streaming implementation.
  static constexpr std::chrono::milliseconds kDefaultBurstInterval{10};

  // A special time_point value representing "never."
  static constexpr Clock::time_point kNever = Clock::time_point::max();

 private:
  struct SenderEntry {
    Ssrc receiver_ssrc;
    Sender* sender;
    Clock::time_point next_rtcp_send_time;
    Clock::time_point next_rtp_send_time;

    // Entries are ordered by the transmission priority (high→low), as implied
    // by their SSRC. See ssrc.h for details.
    bool operator<(const SenderEntry& other) const {
      return ComparePriority(receiver_ssrc, other.receiver_ssrc) < 0;
    }
  };

  using SenderEntries = std::vector<SenderEntry>;

  // Environment::PacketConsumer implementation.
  void OnReceivedPacket(const IPEndpoint& source,
                        Clock::time_point arrival_time,
                        std::vector<uint8_t> packet) final;

  // Helper to return an iterator pointing to the entry corresponding to the
  // given |receiver_ssrc|, or "end" if not found.
  SenderEntries::iterator FindEntry(Ssrc receiver_ssrc);

  // Examine the next send time for all Senders, and decide whether to schedule
  // a burst-send.
  void ScheduleNextBurst();

  // Performs a burst-send of packets. This is called whenever the Alarm fires.
  void SendBurstOfPackets();

  // Send an RTCP packet from each Sender that has one ready, and return the
  // number of packets sent.
  int SendJustTheRtcpPackets(Clock::time_point send_time);

  // Send zero or more RTP packets from each Sender, up to a maximum of
  // |num_packets_to_send|, and return the number of packets sent.
  int SendJustTheRtpPackets(Clock::time_point send_time,
                            int num_packets_to_send);

  // Returns the maximum number of packets to send in one burst, based on the
  // given parameters.
  static int ComputeMaxPacketsPerBurst(
      int max_burst_bitrate,
      int packet_size,
      std::chrono::milliseconds burst_interval);

  // Returns the maximum bitrate inferred by the given parameters.
  static int ComputeMaxBurstBitrate(int packet_size,
                                    int max_packets_per_burst,
                                    std::chrono::milliseconds burst_interval);

  Environment* const environment_;
  const int packet_buffer_size_;
  const std::unique_ptr<uint8_t[]> packet_buffer_;
  const int max_packets_per_burst_;
  const std::chrono::milliseconds burst_interval_;
  const int max_burst_bitrate_;

  // Schedules the task that calls back into this SenderPacketRouter at a later
  // time to send the next burst of packets.
  Alarm alarm_;

  // The current list of Senders and their timing information. This is
  // maintained in order of the priority implied by the Sender SSRC's.
  SenderEntries senders_;

  // The last time a burst of packets was sent. This is used to determine the
  // next burst time.
  Clock::time_point last_burst_time_ = Clock::time_point::min();
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_SENDER_PACKET_ROUTER_H_
