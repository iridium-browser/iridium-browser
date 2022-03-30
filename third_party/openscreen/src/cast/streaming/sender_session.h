// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_SENDER_SESSION_H_
#define CAST_STREAMING_SENDER_SESSION_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cast/common/public/message_port.h"
#include "cast/streaming/answer_messages.h"
#include "cast/streaming/capture_configs.h"
#include "cast/streaming/capture_recommendations.h"
#include "cast/streaming/offer_messages.h"
#include "cast/streaming/remoting_capabilities.h"
#include "cast/streaming/rpc_messenger.h"
#include "cast/streaming/sender.h"
#include "cast/streaming/sender_packet_router.h"
#include "cast/streaming/session_config.h"
#include "cast/streaming/session_messenger.h"
#include "json/value.h"
#include "util/json/json_serialization.h"

namespace openscreen {
namespace cast {

class Environment;
class Sender;

class SenderSession final {
 public:
  // Upon successful negotiation, a set of configured senders is constructed
  // for handling audio and video. Note that either sender may be null.
  struct ConfiguredSenders {
    // In practice, we may have 0, 1, or 2 senders configured, depending
    // on if the device supports audio and video, and if we were able to
    // successfully negotiate a sender configuration.

    // If the sender is audio- or video-only, either of the senders
    // may be nullptr. However, in the majority of cases they will be populated.
    Sender* audio_sender = nullptr;
    AudioCaptureConfig audio_config;

    Sender* video_sender = nullptr;
    VideoCaptureConfig video_config;
  };

  // This struct contains all of the information necessary to begin remoting
  // after we receive the capabilities from the receiver.
  struct RemotingNegotiation {
    ConfiguredSenders senders;

    // The capabilities reported by the connected receiver. NOTE: SenderSession
    // reports the capabilities as-is from the Receiver, so clients concerned
    // about legacy devices, such as pre-1.27 Earth receivers should do
    // a version check when using these capabilities to offer remoting.
    RemotingCapabilities capabilities;
  };

  // The embedder should provide a client for handling negotiation events.
  // The client is required to implement a mirorring handler, and may choose
  // to provide a remoting negotiation if it supports remoting.
  // When the negotiation is complete, the appropriate |On*Negotiated| handler
  // is called.
  class Client {
   public:
    // Called when a new set of senders has been negotiated. This may be
    // called multiple times during a session, once for every time Negotiate()
    // is called on the SenderSession object. The negotiation call also includes
    // capture recommendations that can be used by the sender to provide
    // an optimal video stream for the receiver.
    virtual void OnNegotiated(
        const SenderSession* session,
        ConfiguredSenders senders,
        capture_recommendations::Recommendations capture_recommendations) = 0;

    // Called when a new set of remoting senders has been negotiated. Since
    // remoting is an optional feature, the default behavior here is to leave
    // this method unhandled.
    virtual void OnRemotingNegotiated(const SenderSession* session,
                                      RemotingNegotiation negotiation) {}

    // Called whenever an error occurs. Ends the ongoing session, and the caller
    // must call Negotiate() again if they wish to re-establish streaming.
    virtual void OnError(const SenderSession* session, Error error) = 0;

   protected:
    virtual ~Client();
  };

  // The configuration information required to set up the session.
  struct Configuration {
    // The remote address of the receiver to connect to. NOTE: we do eventually
    // set the remote endpoint on the |environment| object, but only after
    // getting the port information from a successful ANSWER message.
    IPAddress remote_address;

    // The client for notifying of successful negotiations and errors. Required.
    Client* const client;

    // The cast environment used to access operating system resources, such
    // as the UDP socket for RTP/RTCP messaging. Required.
    Environment* environment;

    // The message port used to send streaming control protocol messages.
    MessagePort* message_port;

    // The message source identifier (e.g. this sender).
    std::string message_source_id;

    // The message destination identifier (e.g. the receiver we are connected
    // to).
    std::string message_destination_id;

    // Whether or not the android RTP value hack should be used (for legacy
    // android devices). For more information, see https://crbug.com/631828.
    bool use_android_rtp_hack = true;
  };

  // The SenderSession assumes that the passed in client, environment, and
  // message port persist for at least the lifetime of the SenderSession. If
  // one of these classes needs to be reset, a new SenderSession should be
  // created.
  //
  // |message_source_id| and |message_destination_id| are the local and remote
  // ID, respectively, to use when sending or receiving control messages (e.g.,
  // OFFERs or ANSWERs) over the |message_port|. |message_port|'s SetClient()
  // method will be called.
  explicit SenderSession(Configuration config);
  SenderSession(const SenderSession&) = delete;
  SenderSession(SenderSession&&) noexcept = delete;
  SenderSession& operator=(const SenderSession&) = delete;
  SenderSession& operator=(SenderSession&&) = delete;
  ~SenderSession();

  // Starts a mirroring OFFER/ANSWER exchange with the already configured
  // receiver over the message port. The caller should assume any configured
  // senders become invalid when calling this method.
  Error Negotiate(std::vector<AudioCaptureConfig> audio_configs,
                  std::vector<VideoCaptureConfig> video_configs);

  // Remoting negotiation is actually very similar to mirroring negotiation--
  // an OFFER/ANSWER exchange still occurs, however only one audio and video
  // codec should be presented based on the encoding of the media element that
  // should be remoted. Note: the codec fields in |audio_config| and
  // |video_config| are ignored in favor of |kRemote|.
  Error NegotiateRemoting(AudioCaptureConfig audio_config,
                          VideoCaptureConfig video_config);

  // Get the current network usage (in bits per second). This includes all
  // senders managed by this session, and is a best guess based on receiver
  // feedback. Embedders may use this information to throttle capture devices.
  int GetEstimatedNetworkBandwidth() const;

  // The RPC messenger for this session. NOTE: RPC messages may come at
  // any time from the receiver, so subscriptions to RPC remoting messages
  // should be done before calling |NegotiateRemoting|.
  RpcMessenger* rpc_messenger() { return &rpc_messenger_; }

 private:
  // We store the current negotiation, so that when we get an answer from the
  // receiver we can line up the selected streams with the original
  // configuration.
  struct InProcessNegotiation {
    // The offer, which should always be valid if we have an in process
    // negotiation.
    Offer offer;

    // The configs used to derive the offer.
    std::vector<AudioCaptureConfig> audio_configs;
    std::vector<VideoCaptureConfig> video_configs;

    // The answer message for this negotiation, which may be invalid if we
    // haven't received an answer yet.
    Answer answer;
  };

  // The state of the session.
  enum class State {
    // Not sending content--may be in the middle of negotiation, or just
    // waiting.
    kIdle,

    // Currently mirroring content to a receiver.
    kStreaming,

    // Currently remoting content to a receiver.
    kRemoting
  };

  // Reset the state and tear down the current negotiation/negotiated mirroring
  // or remoting session. After reset, the SenderSession is still connected to
  // the same |remote_address_|, and the |packet_router_| and sequence number
  // will be unchanged.
  void ResetState();

  // Uses the passed in configs and offer to send an OFFER/ANSWER negotiation
  // and cache the new InProcessNavigation.
  Error StartNegotiation(std::vector<AudioCaptureConfig> audio_configs,
                         std::vector<VideoCaptureConfig> video_configs,
                         Offer offer);

  // Specific message type handler methods.
  void OnAnswer(ReceiverMessage message);
  void OnCapabilitiesResponse(ReceiverMessage message);
  void OnRpcMessage(ReceiverMessage message);
  void HandleErrorMessage(ReceiverMessage message, const std::string& text);

  // Used by SpawnSenders to generate a sender for a specific stream.
  std::unique_ptr<Sender> CreateSender(Ssrc receiver_ssrc,
                                       const Stream& stream,
                                       RtpPayloadType type);

  // Helper methods for spawning specific senders from the Answer message.
  void SpawnAudioSender(ConfiguredSenders* senders,
                        Ssrc receiver_ssrc,
                        int send_index,
                        int config_index);
  void SpawnVideoSender(ConfiguredSenders* senders,
                        Ssrc receiver_ssrc,
                        int send_index,
                        int config_index);

  // Spawn a set of configured senders from the currently stored negotiation.
  ConfiguredSenders SpawnSenders(const Answer& answer);

  // Used by the RPC messenger to send outbound messages.
  void SendRpcMessage(std::vector<uint8_t> message_body);

  // This session's configuration.
  Configuration config_;

  // The session messenger, which uses the message port for sending control
  // messages. For message formats, see
  // cast/protocol/castv2/streaming_schema.json.
  SenderSessionMessenger messenger_;

  // The RPC messenger, which uses the session messager for sending RPC messages
  // and handles subscriptions to RPC messages.
  RpcMessenger rpc_messenger_;

  // The packet router used for RTP/RTCP messaging across all senders.
  SenderPacketRouter packet_router_;

  // Each negotiation has its own sequence number, and the receiver replies
  // with the same sequence number that we send. Each message to the receiver
  // advances our current sequence number.
  int current_sequence_number_ = 0;

  // The current negotiation. If present, we are expected an ANSWER from
  // the receiver. If not present, any provided ANSWERS are rejected.
  std::unique_ptr<InProcessNegotiation> current_negotiation_;

  // The current state of the session. Note that the state is intentionally
  // limited. |kStreaming| or |kRemoting| means that we are either starting
  // a negotiation or actively sending to a receiver.
  State state_ = State::kIdle;

  // If the negotiation has succeeded, we store the current audio and video
  // senders used for this session. Either or both may be nullptr.
  std::unique_ptr<Sender> current_audio_sender_;
  std::unique_ptr<Sender> current_video_sender_;
};  // namespace cast

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_SENDER_SESSION_H_
