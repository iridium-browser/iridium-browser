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

// Calculates whether any codecs present in |second| are not present in |first|.
template <typename T>
bool IsMissingCodecs(const std::vector<T>& first,
                     const std::vector<T>& second) {
  if (second.size() > first.size()) {
    return true;
  }

  for (auto codec : second) {
    if (!Contains(first, codec)) {
      return true;
    }
  }

  return false;
}

// Calculates whether the limits defined by |first| are less restrictive than
// those defined by |second|.
// NOTE: These variables are intentionally passed by copy - the function will
// mutate them.
template <typename T>
bool HasLessRestrictiveLimits(std::vector<T> first, std::vector<T> second) {
  // Sort both vectors to allow for element-by-element comparison between the
  // two. All elements with |applies_to_all_codecs| set are sorted to the front.
  std::function<bool(const T&, const T&)> sorter = [](const T& x, const T& y) {
    if (x.applies_to_all_codecs != y.applies_to_all_codecs) {
      return x.applies_to_all_codecs;
    }
    return static_cast<int>(x.codec) < static_cast<int>(y.codec);
  };
  std::sort(first.begin(), first.end(), sorter);
  std::sort(second.begin(), second.end(), sorter);
  auto first_it = first.begin();
  auto second_it = second.begin();

  // |applies_to_all_codecs| is a special case, so handle that first.
  T fake_applies_to_all_codecs_struct;
  fake_applies_to_all_codecs_struct.applies_to_all_codecs = true;
  T* first_applies_to_all_codecs_struct =
      !first.empty() && first.front().applies_to_all_codecs
          ? &(*first_it++)
          : &fake_applies_to_all_codecs_struct;
  T* second_applies_to_all_codecs_struct =
      !second.empty() && second.front().applies_to_all_codecs
          ? &(*second_it++)
          : &fake_applies_to_all_codecs_struct;
  if (!first_applies_to_all_codecs_struct->IsSupersetOf(
          *second_applies_to_all_codecs_struct)) {
    return false;
  }

  // Now all elements of the vectors can be assumed to NOT have
  // |applies_to_all_codecs| set. So iterate through all codecs set in either
  // vector and check that the first has the less restrictive configuration set.
  while (first_it != first.end() || second_it != second.end()) {
    // Calculate the current codec to process, and whether each vector contains
    // an instance of this codec.
    decltype(T::codec) current_codec;
    bool use_first_fake = false;
    bool use_second_fake = false;
    if (first_it == first.end()) {
      current_codec = second_it->codec;
      use_first_fake = true;
    } else if (second_it == second.end()) {
      current_codec = first_it->codec;
      use_second_fake = true;
    } else {
      current_codec = std::min(first_it->codec, second_it->codec);
      use_first_fake = first_it->codec != current_codec;
      use_second_fake = second_it->codec != current_codec;
    }

    // Compare each vector's limit associated with this codec, or compare
    // against the default limits if no such codec limits are set.
    T fake_codecs_struct;
    fake_codecs_struct.codec = current_codec;
    T* first_codec_struct =
        use_first_fake ? &fake_codecs_struct : &(*first_it++);
    T* second_codec_struct =
        use_second_fake ? &fake_codecs_struct : &(*second_it++);
    OSP_DCHECK(!first_codec_struct->applies_to_all_codecs);
    OSP_DCHECK(!second_codec_struct->applies_to_all_codecs);
    if (!first_codec_struct->IsSupersetOf(*second_codec_struct)) {
      return false;
    }
  }

  return true;
}

}  // namespace

ReceiverSession::Client::~Client() = default;

using RemotingPreferences = ReceiverSession::RemotingPreferences;

using Preferences = ReceiverSession::Preferences;

Preferences::Preferences() = default;
Preferences::Preferences(std::vector<VideoCodec> video_codecs,
                         std::vector<AudioCodec> audio_codecs)
    : video_codecs(std::move(video_codecs)),
      audio_codecs(std::move(audio_codecs)) {}

Preferences::Preferences(std::vector<VideoCodec> video_codecs,
                         std::vector<AudioCodec> audio_codecs,
                         std::vector<AudioLimits> audio_limits,
                         std::vector<VideoLimits> video_limits,
                         std::unique_ptr<Display> description)
    : video_codecs(std::move(video_codecs)),
      audio_codecs(std::move(audio_codecs)),
      audio_limits(std::move(audio_limits)),
      video_limits(std::move(video_limits)),
      display_description(std::move(description)) {}

Preferences::Preferences(Preferences&&) noexcept = default;
Preferences& Preferences::operator=(Preferences&&) noexcept = default;

Preferences::Preferences(const Preferences& other) {
  *this = other;
}

Preferences& Preferences::operator=(const Preferences& other) {
  video_codecs = other.video_codecs;
  audio_codecs = other.audio_codecs;
  audio_limits = other.audio_limits;
  video_limits = other.video_limits;
  if (other.display_description) {
    display_description = std::make_unique<Display>(*other.display_description);
  }
  if (other.remoting) {
    remoting = std::make_unique<RemotingPreferences>(*other.remoting);
  }
  return *this;
}

ReceiverSession::ReceiverSession(Client* const client,
                                 Environment* environment,
                                 MessagePort* message_port,
                                 Preferences preferences)
    : client_(client),
      environment_(environment),
      preferences_(std::move(preferences)),
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
      preferences_.video_codecs.begin(), preferences_.video_codecs.end(),
      [](VideoCodec c) { return c == VideoCodec::kNotSpecified; }));
  OSP_DCHECK(!std::any_of(
      preferences_.audio_codecs.begin(), preferences_.audio_codecs.end(),
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
    if (!preferences_.remoting) {
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
  if (preferences_.remoting) {
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
    if (!offer.audio_streams.empty() && !preferences_.audio_codecs.empty()) {
      properties->selected_audio =
          SelectStream(preferences_.audio_codecs, client_, offer.audio_streams);
    }
    if (!offer.video_streams.empty() && !preferences_.video_codecs.empty()) {
      properties->selected_video =
          SelectStream(preferences_.video_codecs, client_, offer.video_streams);
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

    for (const auto& limit : preferences_.audio_limits) {
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

    for (const auto& limit : preferences_.video_limits) {
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
  if (preferences_.display_description) {
    const auto* d = preferences_.display_description.get();
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
  OSP_DCHECK(preferences_.remoting);
  ReceiverCapability capability;
  capability.remoting_version = kSupportedRemotingVersion;

  for (const AudioCodec& codec : preferences_.audio_codecs) {
    capability.media_capabilities.push_back(ToCapability(codec));
  }
  for (const VideoCodec& codec : preferences_.video_codecs) {
    capability.media_capabilities.push_back(ToCapability(codec));
  }

  if (preferences_.remoting->supports_chrome_audio_codecs) {
    capability.media_capabilities.push_back(MediaCapability::kAudio);
  }
  if (preferences_.remoting->supports_4k) {
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

bool ReceiverSession::VideoLimits::IsSupersetOf(
    const ReceiverSession::VideoLimits& second) const {
  return (applies_to_all_codecs == second.applies_to_all_codecs) &&
         (applies_to_all_codecs || codec == second.codec) &&
         (max_pixels_per_second >= second.max_pixels_per_second) &&
         (min_bit_rate <= second.min_bit_rate) &&
         (max_bit_rate >= second.max_bit_rate) &&
         (max_delay >= second.max_delay) &&
         (max_dimensions.IsSupersetOf(second.max_dimensions));
}

bool ReceiverSession::AudioLimits::IsSupersetOf(
    const ReceiverSession::AudioLimits& second) const {
  return (applies_to_all_codecs == second.applies_to_all_codecs) &&
         (applies_to_all_codecs || codec == second.codec) &&
         (max_sample_rate >= second.max_sample_rate) &&
         (max_channels >= second.max_channels) &&
         (min_bit_rate <= second.min_bit_rate) &&
         (max_bit_rate >= second.max_bit_rate) &&
         (max_delay >= second.max_delay);
}

bool ReceiverSession::Display::IsSupersetOf(
    const ReceiverSession::Display& other) const {
  return dimensions.IsSupersetOf(other.dimensions) &&
         (can_scale_content || !other.can_scale_content);
}

bool ReceiverSession::RemotingPreferences::IsSupersetOf(
    const ReceiverSession::RemotingPreferences& other) const {
  return (supports_chrome_audio_codecs ||
          !other.supports_chrome_audio_codecs) &&
         (supports_4k || !other.supports_4k);
}

bool ReceiverSession::Preferences::IsSupersetOf(
    const ReceiverSession::Preferences& other) const {
  // Check simple cases first.
  if ((!!display_description != !!other.display_description) ||
      (display_description &&
       !display_description->IsSupersetOf(*other.display_description))) {
    return false;
  } else if (other.remoting &&
             (!remoting || !remoting->IsSupersetOf(*other.remoting))) {
    return false;
  }

  // Then check set codecs.
  if (IsMissingCodecs(video_codecs, other.video_codecs) ||
      IsMissingCodecs(audio_codecs, other.audio_codecs)) {
    return false;
  }

  // Then check limits. Do this last because it's the most resource intensive to
  // check.
  return HasLessRestrictiveLimits(video_limits, other.video_limits) &&
         HasLessRestrictiveLimits(audio_limits, other.audio_limits);
}

}  // namespace cast
}  // namespace openscreen
