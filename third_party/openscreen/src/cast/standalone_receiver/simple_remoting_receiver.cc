// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/simple_remoting_receiver.h"

#include <utility>

#include "cast/streaming/message_fields.h"
#include "cast/streaming/remoting.pb.h"

namespace openscreen {
namespace cast {

namespace {

VideoCodec ParseProtoCodec(VideoDecoderConfig::Codec value) {
  switch (value) {
    case VideoDecoderConfig_Codec_kCodecHEVC:
      return VideoCodec::kHevc;

    case VideoDecoderConfig_Codec_kCodecH264:
      return VideoCodec::kH264;

    case VideoDecoderConfig_Codec_kCodecVP8:
      return VideoCodec::kVp8;

    case VideoDecoderConfig_Codec_kCodecVP9:
      return VideoCodec::kVp9;

    case VideoDecoderConfig_Codec_kCodecAV1:
      return VideoCodec::kAv1;

    default:
      return VideoCodec::kNotSpecified;
  }
}

AudioCodec ParseProtoCodec(AudioDecoderConfig::Codec value) {
  switch (value) {
    case AudioDecoderConfig_Codec_kCodecAAC:
      return AudioCodec::kAac;

    case AudioDecoderConfig_Codec_kCodecOpus:
      return AudioCodec::kOpus;

    default:
      return AudioCodec::kNotSpecified;
  }
}

}  // namespace

SimpleRemotingReceiver::SimpleRemotingReceiver(RpcMessenger* messenger)
    : messenger_(messenger) {
  messenger_->RegisterMessageReceiverCallback(
      RpcMessenger::kFirstHandle, [this](std::unique_ptr<RpcMessage> message) {
        this->OnInitializeCallbackMessage(std::move(message));
      });
}

SimpleRemotingReceiver::~SimpleRemotingReceiver() {
  messenger_->UnregisterMessageReceiverCallback(RpcMessenger::kFirstHandle);
}

void SimpleRemotingReceiver::SendInitializeMessage(
    SimpleRemotingReceiver::InitializeCallback initialize_cb) {
  initialize_cb_ = std::move(initialize_cb);

  OSP_DVLOG
      << "Indicating to the sender we are ready for remoting initialization.";
  openscreen::cast::RpcMessage rpc;
  rpc.set_handle(RpcMessenger::kAcquireRendererHandle);
  rpc.set_proc(openscreen::cast::RpcMessage::RPC_DS_INITIALIZE);

  // The initialize message contains the handle to be used for sending the
  // initialization callback message.
  rpc.set_integer_value(RpcMessenger::kFirstHandle);
  messenger_->SendMessageToRemote(rpc);
}

void SimpleRemotingReceiver::SendPlaybackRateMessage(double playback_rate) {
  openscreen::cast::RpcMessage rpc;
  rpc.set_handle(RpcMessenger::kAcquireRendererHandle);
  rpc.set_proc(openscreen::cast::RpcMessage::RPC_R_SETPLAYBACKRATE);
  rpc.set_double_value(playback_rate);
  messenger_->SendMessageToRemote(rpc);
}

void SimpleRemotingReceiver::OnInitializeCallbackMessage(
    std::unique_ptr<RpcMessage> message) {
  OSP_DCHECK(message->proc() == RpcMessage::RPC_DS_INITIALIZE_CALLBACK);
  if (!initialize_cb_) {
    OSP_DLOG_INFO << "Received an initialization callback message but no "
                     "callback was set.";
    return;
  }

  const DemuxerStreamInitializeCallback& callback_message =
      message->demuxerstream_initializecb_rpc();
  const auto audio_codec =
      callback_message.has_audio_decoder_config()
          ? ParseProtoCodec(callback_message.audio_decoder_config().codec())
          : AudioCodec::kNotSpecified;
  const auto video_codec =
      callback_message.has_video_decoder_config()
          ? ParseProtoCodec(callback_message.video_decoder_config().codec())
          : VideoCodec::kNotSpecified;

  OSP_DLOG_INFO << "Initializing remoting with audio codec "
                << CodecToString(audio_codec) << " and video codec "
                << CodecToString(video_codec);
  initialize_cb_(audio_codec, video_codec);
  initialize_cb_ = nullptr;
}

}  // namespace cast
}  // namespace openscreen
