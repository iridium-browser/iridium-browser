// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver_session.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "cast/common/channel/message_util.h"
#include "cast/common/public/message_port.h"
#include "cast/streaming/answer_messages.h"
#include "cast/streaming/environment.h"
#include "cast/streaming/message_fields.h"
#include "cast/streaming/offer_messages.h"
#include "cast/streaming/receiver.h"
#include "cast/streaming/sender_message.h"
#include "util/json/json_helpers.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {
namespace {

template <typename Stream, typename Codec>
std::unique_ptr<Stream> SelectStream(
    const std::vector<Codec>& preferred_codecs,
    ReceiverSession::Client* client,
    const std::vector<Stream>& offered_streams) {
  for (auto codec : preferred_codecs) {
    for (const Stream& offered_stream : offered_streams) {
      if (offered_stream.codec == codec &&
          (offered_stream.stream.codec_parameter.empty() ||
           client->SupportsCodecParameter(
               offered_stream.stream.codec_parameter))) {
        OSP_VLOG << "Selected " << CodecToString(codec)
                 << " as codec for streaming";
        return std::make_unique<Stream>(offered_stream);
      }
    }
  }
  return nullptr;
}

MediaCapability ToCapability(AudioCodec codec) {
  switch (codec) {
    case AudioCodec::kAac:
      return MediaCapability::kAac;
    case AudioCodec::kOpus:
      return MediaCapability::kOpus;
    default:
      OSP_DLOG_FATAL << "Invalid audio codec: " << static_cast<int>(codec);
      OSP_NOTREACHED();
  }
}

MediaCapability ToCapability(VideoCodec codec) {
  switch (codec) {
    case VideoCodec::kVp8:
      return MediaCapability::kVp8;
    case VideoCodec::kVp9:
      return MediaCapability::kVp9;
    case VideoCodec::kH264:
      return MediaCapability::kH264;
    case VideoCodec::kHevc:
      return MediaCapability::kHevc;
    case VideoCodec::kAv1:
      return MediaCapability::kAv1;
    default:
      OSP_DLOG_FATAL << "Invalid video codec: " << static_cast<int>(codec);
      OSP_NOTREACHED();
  }
}

}  // namespace

ReceiverSession::Client::~Client() = default;

ReceiverSession::ReceiverSession(Client* const client,
                                 Environment* environment,
                                 MessagePort* message_port,
                                 ReceiverConstraints constraints)
    : client_(client),
      environment_(environment),
      constraints_(std::move(constraints)),
      session_id_(MakeUniqueSessionId("streaming_receiver")),
      messenger_(message_port,
                 session_id_,
                 [this](Error error) {
                   OSP_DLOG_WARN << "Got a session messenger error: " << error;
                   client_->OnError(this, error);
                 }),
      packet_router_(environment_) {
  OSP_DCHECK(client_);
  OSP_DCHECK(environment_);

  OSP_DCHECK(!std::any_of(
      constraints_.video_codecs.begin(), constraints_.video_codecs.end(),
      [](VideoCodec c) { return c == VideoCodec::kNotSpecified; }));
  OSP_DCHECK(!std::any_of(
      constraints_.audio_codecs.begin(), constraints_.audio_codecs.end(),
      [](AudioCodec c) { return c == AudioCodec::kNotSpecified; }));

  messenger_.SetHandler(
      SenderMessage::Type::kOffer,
      [this](const std::string& sender_id, SenderMessage message) {
        OnOffer(sender_id, std::move(message));
      });
  messenger_.SetHandler(
      SenderMessage::Type::kGetCapabilities,
      [this](const std::string& sender_id, SenderMessage message) {
        OnCapabilitiesRequest(sender_id, std::move(message));
      });
  messenger_.SetHandler(
      SenderMessage::Type::kRpc,
      [this](const std::string& sender_id, SenderMessage message) {
        this->OnRpcMessage(sender_id, std::move(message));
      });
  environment_->SetSocketSubscriber(this);
}

ReceiverSession::~ReceiverSession() {
  ResetReceivers(Client::kEndOfSession);
}

void ReceiverSession::OnSocketReady() {
  if (pending_offer_) {
    InitializeSession(*pending_offer_);
    pending_offer_.reset();
  }
}

void ReceiverSession::OnSocketInvalid(Error error) {
  if (pending_offer_) {
    SendErrorAnswerReply(pending_offer_->sender_id,
                         pending_offer_->sequence_number, error);
    pending_offer_.reset();
  }

  client_->OnError(this,
                   Error(Error::Code::kSocketFailure,
                         "The environment is invalid and should be replaced."));
}

bool ReceiverSession::PendingOffer::IsValid() const {
  return (selected_audio || selected_video) && !sender_id.empty() &&
         sequence_number >= 0;
}

void ReceiverSession::OnOffer(const std::string& sender_id,
                              SenderMessage message) {
  // We just drop offers we can't respond to. Note that libcast senders will
  // always send a strictly positive sequence numbers, but zero is permitted
  // by the spec.
  if (message.sequence_number < 0) {
    OSP_DLOG_WARN
        << "Dropping offer with missing sequence number, can't respond";
    return;
  }

  if (!message.valid) {
    const Error error(Error::Code::kParameterInvalid,
                      "Failed to parse malformed OFFER");
    SendErrorAnswerReply(sender_id, message.sequence_number, error);
    client_->OnError(this, error);
    return;
  }

  auto properties = std::make_unique<PendingOffer>();
  properties->sender_id = sender_id;
  properties->sequence_number = message.sequence_number;

  const Offer& offer = absl::get<Offer>(message.body);

  if (offer.cast_mode == CastMode::kRemoting) {
    if (!constraints_.remoting) {
      SendErrorAnswerReply(
          sender_id, message.sequence_number,

          Error(Error::Code::kRemotingNotSupported,
                "This receiver does not have remoting enabled."));
      return;
    }
  }

  properties->mode = offer.cast_mode;
  SelectStreams(offer, properties.get());
  if (!properties->IsValid()) {
    SendErrorAnswerReply(sender_id, message.sequence_number,
                         Error(Error::Code::kNoStreamSelected,
                               "Failed to select any streams from OFFER"));
    return;
  }

  // Finally, before we update the pending offer let's make sure we don't
  // already have one.
  if (pending_offer_) {
    SendErrorAnswerReply(
        pending_offer_->sender_id, pending_offer_->sequence_number,
        Error(Error::Code::kInterrupted,
              "Received a new OFFER before negotiation could complete."));
    pending_offer_.reset();
  }

  switch (environment_->socket_state()) {
    // If the environment is ready or in a bad state, we can respond
    // immediately.
    case Environment::SocketState::kInvalid:
      SendErrorAnswerReply(
          sender_id, message.sequence_number,
          Error(Error::Code::kSocketClosedFailure,
                "UDP socket is closed, likely due to a bind error."));
      break;

    case Environment::SocketState::kReady:
      InitializeSession(*properties);
      break;

    // Else we need to store the properties we just created until we get a
    // ready or error event.
    case Environment::SocketState::kStarting:
      pending_offer_ = std::move(properties);
      break;
  }
}

void ReceiverSession::OnCapabilitiesRequest(const std::string& sender_id,
                                            SenderMessage message) {
  if (message.sequence_number < 0) {
    OSP_DLOG_WARN
        << "Dropping offer with missing sequence number, can't respond";
    return;
  }

  ReceiverMessage response{
      ReceiverMessage::Type::kCapabilitiesResponse, message.sequence_number,
      true /* valid */
  };
  if (constraints_.remoting) {
    response.body = CreateRemotingCapabilityV2();
  } else {
    response.valid = false;
    response.body = ReceiverError(Error::Code::kRemotingNotSupported,
                                  "Remoting is not supported");
  }

  // NOTE: we respond to any arbitrary sender here, to allow sender to get
  // capabilities before making an OFFER.
  const Error result = messenger_.SendMessage(sender_id, std::move(response));
  if (!result.ok()) {
    client_->OnError(this, std::move(result));
  }
}

void ReceiverSession::OnRpcMessage(const std::string& sender_id,
                                   SenderMessage message) {
  if (!message.valid) {
    OSP_DLOG_WARN
        << "Bad RPC message. This may or may not represent a serious problem.";
    return;
  }

  if (sender_id != negotiated_sender_id_) {
    OSP_DLOG_INFO << "Received an RPC message from sender " << sender_id
                  << "--which we haven't negotiated with, dropping.";
    return;
  }

  const auto& body = absl::get<std::vector<uint8_t>>(message.body);
  if (!rpc_messenger_) {
    OSP_DLOG_INFO << "Received an RPC message without having a messenger.";
    return;
  }
  rpc_messenger_->ProcessMessageFromRemote(body.data(), body.size());
}

void ReceiverSession::SendRpcMessage(std::vector<uint8_t> message) {
  if (negotiated_sender_id_.empty()) {
    OSP_DLOG_WARN
        << "Can't send an RPC message without a currently negotiated session.";
    return;
  }

  const Error error = messenger_.SendMessage(
      negotiated_sender_id_,
      ReceiverMessage{ReceiverMessage::Type::kRpc, -1, true /* valid */,
                      std::move(message)});

  if (!error.ok()) {
    OSP_LOG_WARN << "Failed to send RPC message: " << error;
  }
}

void ReceiverSession::SelectStreams(const Offer& offer,
                                    PendingOffer* properties) {
  if (offer.cast_mode == CastMode::kMirroring) {
    if (!offer.audio_streams.empty() && !constraints_.audio_codecs.empty()) {
      properties->selected_audio =
          SelectStream(constraints_.audio_codecs, client_, offer.audio_streams);
    }
    if (!offer.video_streams.empty() && !constraints_.video_codecs.empty()) {
      properties->selected_video =
          SelectStream(constraints_.video_codecs, client_, offer.video_streams);
    }
  } else {
    OSP_DCHECK(offer.cast_mode == CastMode::kRemoting);

    if (offer.audio_streams.size() == 1) {
      properties->selected_audio =
          std::make_unique<AudioStream>(offer.audio_streams[0]);
    }
    if (offer.video_streams.size() == 1) {
      properties->selected_video =
          std::make_unique<VideoStream>(offer.video_streams[0]);
    }
  }
}

void ReceiverSession::InitializeSession(const PendingOffer& properties) {
  Answer answer = ConstructAnswer(properties);
  if (!answer.IsValid()) {
    // If the answer message is invalid, there is no point in setting up a
    // negotiation because the sender won't be able to connect to it.
    SendErrorAnswerReply(properties.sender_id, properties.sequence_number,
                         Error(Error::Code::kParameterInvalid,
                               "Failed to construct an ANSWER message"));
    return;
  }

  // Only spawn receivers if we know we have a valid answer message.
  ConfiguredReceivers receivers = SpawnReceivers(properties);
  negotiated_sender_id_ = properties.sender_id;
  if (properties.mode == CastMode::kMirroring) {
    client_->OnNegotiated(this, std::move(receivers));
  } else {
    rpc_messenger_ =
        std::make_unique<RpcMessenger>([this](std::vector<uint8_t> message) {
          this->SendRpcMessage(std::move(message));
        });
    client_->OnRemotingNegotiated(
        this, RemotingNegotiation{std::move(receivers), rpc_messenger_.get()});
  }

  const Error result = messenger_.SendMessage(
      negotiated_sender_id_,
      ReceiverMessage{ReceiverMessage::Type::kAnswer,
                      properties.sequence_number, true /* valid */,
                      std::move(answer)});
  if (!result.ok()) {
    client_->OnError(this, std::move(result));
  }
}

std::unique_ptr<Receiver> ReceiverSession::ConstructReceiver(
    const Stream& stream) {
  // Session config is currently only for mirroring.
  SessionConfig config = {stream.ssrc,         stream.ssrc + 1,
                          stream.rtp_timebase, stream.channels,
                          stream.target_delay, stream.aes_key,
                          stream.aes_iv_mask,  /* is_pli_enabled */ true};
  if (!config.IsValid()) {
    return nullptr;
  }
  return std::make_unique<Receiver>(environment_, &packet_router_,
                                    std::move(config));
}

ReceiverSession::ConfiguredReceivers ReceiverSession::SpawnReceivers(
    const PendingOffer& properties) {
  OSP_DCHECK(properties.IsValid());
  ResetReceivers(Client::kRenegotiated);

  AudioCaptureConfig audio_config;
  if (properties.selected_audio) {
    current_audio_receiver_ =
        ConstructReceiver(properties.selected_audio->stream);
    audio_config =
        AudioCaptureConfig{properties.selected_audio->codec,
                           properties.selected_audio->stream.channels,
                           properties.selected_audio->bit_rate,
                           properties.selected_audio->stream.rtp_timebase,
                           properties.selected_audio->stream.target_delay,
                           properties.selected_audio->stream.codec_parameter};
  }

  VideoCaptureConfig video_config;
  if (properties.selected_video) {
    current_video_receiver_ =
        ConstructReceiver(properties.selected_video->stream);
    video_config =
        VideoCaptureConfig{properties.selected_video->codec,
                           properties.selected_video->max_frame_rate,
                           properties.selected_video->max_bit_rate,
                           properties.selected_video->resolutions,
                           properties.selected_video->stream.target_delay,
                           properties.selected_video->stream.codec_parameter};
  }

  return ConfiguredReceivers{current_audio_receiver_.get(),
                             std::move(audio_config),
                             current_video_receiver_.get(),
                             std::move(video_config), properties.sender_id};
}

void ReceiverSession::ResetReceivers(Client::ReceiversDestroyingReason reason) {
  if (current_video_receiver_ || current_audio_receiver_) {
    client_->OnReceiversDestroying(this, reason);
    current_audio_receiver_.reset();
    current_video_receiver_.reset();
    rpc_messenger_.reset();
  }
}

Answer ReceiverSession::ConstructAnswer(const PendingOffer& properties) {
  OSP_DCHECK(properties.IsValid());

  std::vector<int> stream_indexes;
  std::vector<Ssrc> stream_ssrcs;
  Constraints constraints;
  if (properties.selected_audio) {
    stream_indexes.push_back(properties.selected_audio->stream.index);
    stream_ssrcs.push_back(properties.selected_audio->stream.ssrc + 1);

    for (const auto& limit : constraints_.audio_limits) {
      if (limit.codec == properties.selected_audio->codec ||
          limit.applies_to_all_codecs) {
        constraints.audio = AudioConstraints{
            limit.max_sample_rate, limit.max_channels, limit.min_bit_rate,
            limit.max_bit_rate,    limit.max_delay,
        };
        break;
      }
    }
  }

  if (properties.selected_video) {
    stream_indexes.push_back(properties.selected_video->stream.index);
    stream_ssrcs.push_back(properties.selected_video->stream.ssrc + 1);

    for (const auto& limit : constraints_.video_limits) {
      if (limit.codec == properties.selected_video->codec ||
          limit.applies_to_all_codecs) {
        constraints.video = VideoConstraints{
            limit.max_pixels_per_second, absl::nullopt, /* min dimensions */
            limit.max_dimensions,        limit.min_bit_rate,
            limit.max_bit_rate,          limit.max_delay,
        };
        break;
      }
    }
  }

  absl::optional<DisplayDescription> display;
  if (constraints_.display_description) {
    const auto* d = constraints_.display_description.get();
    display = DisplayDescription{d->dimensions, absl::nullopt,
                                 d->can_scale_content
                                     ? AspectRatioConstraint::kVariable
                                     : AspectRatioConstraint::kFixed};
  }

  // Only set the constraints in the answer if they are valid (meaning we
  // successfully found limits above).
  absl::optional<Constraints> answer_constraints;
  if (constraints.IsValid()) {
    answer_constraints = std::move(constraints);
  }
  return Answer{environment_->GetBoundLocalEndpoint().port,
                std::move(stream_indexes), std::move(stream_ssrcs),
                answer_constraints, std::move(display)};
}

ReceiverCapability ReceiverSession::CreateRemotingCapabilityV2() {
  // If we don't support remoting, there is no reason to respond to
  // capability requests--they are not used for mirroring.
  OSP_DCHECK(constraints_.remoting);
  ReceiverCapability capability;
  capability.remoting_version = kSupportedRemotingVersion;

  for (const AudioCodec& codec : constraints_.audio_codecs) {
    capability.media_capabilities.push_back(ToCapability(codec));
  }
  for (const VideoCodec& codec : constraints_.video_codecs) {
    capability.media_capabilities.push_back(ToCapability(codec));
  }

  if (constraints_.remoting->supports_chrome_audio_codecs) {
    capability.media_capabilities.push_back(MediaCapability::kAudio);
  }
  if (constraints_.remoting->supports_4k) {
    capability.media_capabilities.push_back(MediaCapability::k4k);
  }
  return capability;
}

void ReceiverSession::SendErrorAnswerReply(const std::string& sender_id,
                                           int sequence_number,
                                           Error error) {
  OSP_DLOG_WARN << error;
  const Error result = messenger_.SendMessage(
      sender_id,
      ReceiverMessage{ReceiverMessage::Type::kAnswer, sequence_number,
                      false /* valid */, ReceiverError(error)});
  if (!result.ok()) {
    client_->OnError(this, std::move(result));
  }
}

}  // namespace cast
}  // namespace openscreen
