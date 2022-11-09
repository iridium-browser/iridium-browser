// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_REMOTING_SENDER_H_
#define CAST_STANDALONE_SENDER_REMOTING_SENDER_H_

#include <memory>

#include "cast/streaming/constants.h"
#include "cast/streaming/rpc_messenger.h"

namespace openscreen {
namespace cast {

// This class behaves like a pared-down version of Chrome's StreamProvider (see
// https://source.chromium.org/chromium/chromium/src/+/main:media/remoting/stream_provider.h
// ). Instead of fully managing a media::DemuxerStream however, it just provides
// an RPC initialization routine that notifies the standalone receiver's
// SimpleRemotingReceiver instance (if configured) that initialization has been
// complete and what codecs were selected.
//
// Due to the sheer complexity of remoting, we don't have a fully functional
// implementation of remoting in the standalone_* components, instead Chrome is
// the reference implementation and we have these simple classes to exercise
// the public APIs.
class RemotingSender {
 public:
  // The remoting sender expects a valid client to handle received messages.
  class Client {
   public:
    virtual ~Client();

    // Executed when we receive the initialize message from the receiver.
    virtual void OnReady() = 0;

    // Executed when we receive a playback rate message from the receiver.
    virtual void OnPlaybackRateChange(double rate) = 0;
  };

  RemotingSender(RpcMessenger& messenger,
                 AudioCodec audio_codec,
                 VideoCodec video_codec,
                 Client* client);
  ~RemotingSender();

 private:
  // Helper for parsing any received RPC messages.
  void OnMessage(const RpcMessage& message);
  void OnInitializeMessage(const RpcMessage& message);
  void OnPlaybackRateMessage(const RpcMessage& message);

  // The messenger is the only caller of OnInitializeMessage, so there are no
  // lifetime concerns. However, if this class outlives |messenger_|, it will
  // no longer receive initialization messages.
  RpcMessenger& messenger_;

  // Unlike in Chrome, here we should know the video and audio codecs before any
  // of the remoting code gets set up, and for simplicity's sake we can only
  // populate the AudioDecoderConfig and VideoDecoderConfig objects with the
  // codecs and use the rest of the fields as-is from the OFFER/ANSWER exchange.
  const AudioCodec audio_codec_;
  const VideoCodec video_codec_;

  Client* client_;

  // The initialization message from the receiver contains the handle the
  // callback should go to.
  RpcMessenger::Handle receiver_handle_ = RpcMessenger::kInvalidHandle;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_REMOTING_SENDER_H_
