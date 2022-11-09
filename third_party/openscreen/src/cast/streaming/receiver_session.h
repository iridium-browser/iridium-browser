// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_RECEIVER_SESSION_H_
#define CAST_STREAMING_RECEIVER_SESSION_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cast/common/public/message_port.h"
#include "cast/streaming/capture_configs.h"
#include "cast/streaming/constants.h"
#include "cast/streaming/offer_messages.h"
#include "cast/streaming/receiver_constraints.h"
#include "cast/streaming/receiver_packet_router.h"
#include "cast/streaming/resolution.h"
#include "cast/streaming/rpc_messenger.h"
#include "cast/streaming/sender_message.h"
#include "cast/streaming/session_config.h"
#include "cast/streaming/session_messenger.h"

namespace openscreen {
namespace cast {

class Environment;
class Receiver;

// This class is responsible for listening for streaming requests from Cast
// Sender devices, then negotiating capture constraints and instantiating audio
// and video Receiver objects.
//   The owner of this session is expected to provide a client for
// updates, an environment for getting UDP socket information (as well as
// other OS dependencies), and a set of constraints to be used for
// negotiation.
//
// NOTE: In some cases, the session initialization may be pending waiting for
// the UDP socket to be ready. In this case, the receivers and the answer
// message will not be configured and sent until the UDP socket has finished
// binding.
class ReceiverSession final : public Environment::SocketSubscriber {
 public:
  // Upon successful negotiation, a set of configured receivers is constructed
  // for handling audio and video. Note that either receiver may be null.
  struct ConfiguredReceivers {
    // In practice, we may have 0, 1, or 2 receivers configured, depending
    // on if the device supports audio and video, and if we were able to
    // successfully negotiate a receiver configuration.

    // NOTES ON LIFETIMES: The audio and video Receiver pointers are owned by
    // ReceiverSession, not the Client, and references to these pointers must be
    // cleared before a call to Client::OnReceiversDestroying() returns.

    // If the receiver is audio- or video-only, or we failed to negotiate
    // an acceptable session configuration with the sender, then either of the
    // receivers may be nullptr. In this case, the associated config is default
    // initialized and should be ignored.
    Receiver* audio_receiver;
    AudioCaptureConfig audio_config;

    Receiver* video_receiver;
    VideoCaptureConfig video_config;

    // The ID of the sender that this set of receivers was configured to
    // communicate with.
    std::string sender_id;
  };

  // This struct contains all of the information necessary to begin remoting
  // once we get a remoting request from a Sender.
  struct RemotingNegotiation {
    // The configured receivers set to be used for handling audio and
    // video streams. Unlike in the general streaming case, when we are remoting
    // we don't know the codec and other information about the stream until
    // the sender provices that information through the
    // DemuxerStreamInitializeCallback RPC method.
    ConfiguredReceivers receivers;

    // The RPC messenger to be used for subscribing to remoting proto messages.
    // Unlike the SenderSession API, the RPC messenger is negotiation specific.
    // The messenger is torn down when |OnReceiversDestroying| is called, and
    // is owned by the ReceiverSession.
    RpcMessenger* messenger;
  };

  // The embedder should provide a client for handling connections.
  // When a connection is established, the OnNegotiated callback is called.
  class Client {
   public:
    // Currently we only care about the session ending or being renegotiated,
    // which means that we don't have to tear down as much state.
    enum ReceiversDestroyingReason { kEndOfSession, kRenegotiated };

    // Called when a set of streaming receivers has been negotiated. Both this
    // and |OnRemotingNegotiated| may be called repeatedly as negotiations occur
    // through the life of a session.
    virtual void OnNegotiated(const ReceiverSession* session,
                              ConfiguredReceivers receivers) = 0;

    // Called when a set of remoting receivers has been negotiated. This will
    // only be called if |RemotingConstraints| are provided as part of
    // constructing the ReceiverSession object.
    virtual void OnRemotingNegotiated(const ReceiverSession* session,
                                      RemotingNegotiation negotiation) {}

    // Called immediately preceding the destruction of this session's receivers.
    // If |reason| is |kEndOfSession|, OnNegotiated() will never be called
    // again; if it is |kRenegotiated|, OnNegotiated() will be called again
    // soon with a new set of Receivers to use.
    //
    // Before returning, the implementation must ensure that all references to
    // the Receivers, from the last call to OnNegotiated(), have been cleared.
    virtual void OnReceiversDestroying(const ReceiverSession* session,
                                       ReceiversDestroyingReason reason) = 0;

    // Called whenever an error that the client may care about occurs.
    // Recoverable errors are usually logged by the receiver session instead
    // of reported here.
    virtual void OnError(const ReceiverSession* session, Error error) = 0;

    // Called to verify whether a given codec parameter is supported by
    // this client. If not overriden, this always assumes true.
    // This method is used only for secondary matching, e.g.
    // if you don't add VideoCodec::kHevc to the VideoCaptureConfig, then
    // supporting codec parameter "hev1.1.6.L153.B0" does not matter.
    //
    // The codec parameter support callback is optional, however if provided
    // then any offered streams that have a non-empty codec parameter field must
    // match. If a stream does not have a codec parameter, this callback will
    // not be called.
    virtual bool SupportsCodecParameter(const std::string& parameter) {
      return true;
    }

   protected:
    virtual ~Client();
  };

  // TODO(crbug.com/1220176): Remove type aliases once downstream has been
  // updated with new type names.
  using Display = openscreen::cast::Display;
  using AudioLimits = openscreen::cast::AudioLimits;
  using VideoLimits = openscreen::cast::VideoLimits;
  using RemotingPreferences = openscreen::cast::RemotingConstraints;
  using Preferences = openscreen::cast::ReceiverConstraints;

  ReceiverSession(Client* const client,
                  Environment* environment,
                  MessagePort* message_port,
                  ReceiverConstraints constraints);
  ReceiverSession(const ReceiverSession&) = delete;
  ReceiverSession(ReceiverSession&&) noexcept = delete;
  ReceiverSession& operator=(const ReceiverSession&) = delete;
  ReceiverSession& operator=(ReceiverSession&&) = delete;
  ~ReceiverSession() override;

  const std::string& session_id() const { return session_id_; }

  // Environment::SocketSubscriber event callbacks.
  void OnSocketReady() override;
  void OnSocketInvalid(Error error) override;

 private:
  // In some cases, such as waiting for the UDP socket to be bound, we
  // may have a pending session that cannot start yet. This class provides
  // all necessary info to instantiate a session.
  struct PendingOffer {
    // The cast mode the OFFER was sent for.
    CastMode mode;

    // The sender that provided the OFFER.
    std::string sender_id;

    // The selected audio and video streams from the original OFFER message.
    std::unique_ptr<AudioStream> selected_audio;
    std::unique_ptr<VideoStream> selected_video;

    // The sequence number of the OFFER that produced these properties.
    int sequence_number;

    // To be valid either the audio or video must be selected, and we must
    // have a sequence number we can reference.
    bool IsValid() const;
  };

  // Specific message type handler methods.
  void OnOffer(const std::string& sender_id, SenderMessage message);
  void OnCapabilitiesRequest(const std::string& sender_id,
                             SenderMessage message);
  void OnRpcMessage(const std::string& sender_id, SenderMessage message);

  // Sends an RPC message to the currently negotiated sender.
  void SendRpcMessage(std::vector<uint8_t> message);

  // Selects streams from an offer based on its configuration, and sets
  // them in the session properties.
  void SelectStreams(const Offer& offer, PendingOffer* properties);

  // Creates receivers and sends an appropriate Answer message using the
  // session properties.
  void InitializeSession(const PendingOffer& properties);

  // Used by SpawnReceivers to generate a receiver for a specific stream.
  std::unique_ptr<Receiver> ConstructReceiver(const Stream& stream);

  // Creates a set of configured receivers from a given pair of audio and
  // video streams. NOTE: either audio or video may be null, but not both.
  ConfiguredReceivers SpawnReceivers(const PendingOffer& properties);

  // Creates an ANSWER object. Assumes at least one stream is not nullptr.
  Answer ConstructAnswer(const PendingOffer& properties);

  // Creates a ReceiverCapability version 2 object. This will be deprecated
  // as part of https://issuetracker.google.com/184429130.
  ReceiverCapability CreateRemotingCapabilityV2();

  // Handles resetting receivers and notifying the client.
  void ResetReceivers(Client::ReceiversDestroyingReason reason);

  // Sends an error answer reply and notifies the client of the error.
  void SendErrorAnswerReply(const std::string& sender_id,
                            int sequence_number,
                            Error error);

  Client* const client_;
  Environment* const environment_;
  const ReceiverConstraints constraints_;

  // The sender_id of this session.
  const std::string session_id_;

  // The ID of the sender that has the current negotiation. We ignore
  // RPC messages from other senders.
  std::string negotiated_sender_id_;

  // The session messenger used for the lifetime of this session.
  ReceiverSessionMessenger messenger_;

  // The packet router to be used for all Receivers spawned by this session.
  ReceiverPacketRouter packet_router_;

  // Any session pending while the UDP socket is being bound.
  std::unique_ptr<PendingOffer> pending_offer_;

  // The negotiated receivers we own, clients are notified of destruction
  // through |Client::OnReceiversDestroying|.
  std::unique_ptr<Receiver> current_audio_receiver_;
  std::unique_ptr<Receiver> current_video_receiver_;

  // If remoting, we store the RpcMessenger used by the embedder to send RPC
  // messages from the remoting protobuf specification.
  std::unique_ptr<RpcMessenger> rpc_messenger_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_RECEIVER_SESSION_H_
