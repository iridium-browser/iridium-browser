// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_RECEIVER_SIMPLE_REMOTING_RECEIVER_H_
#define CAST_STANDALONE_RECEIVER_SIMPLE_REMOTING_RECEIVER_H_

#include <functional>
#include <memory>

#include "cast/streaming/constants.h"
#include "cast/streaming/rpc_messenger.h"

namespace openscreen {
namespace cast {

// This class behaves like a pared-down version of Chrome's DemuxerStreamAdapter
// (see
// https://source.chromium.org/chromium/chromium/src/+/main:/media/remoting/demuxer_stream_adapter.h
// ). Instead of providing a full adapter implementation, it just provides a
// callback register that can be used to notify a component when the
// RemotingProvider sends an initialization message with audio and video codec
// information.
//
// Due to the sheer complexity of remoting, we don't have a fully functional
// implementation of remoting in the standalone_* components, instead Chrome is
// the reference implementation and we have these simple classes to exercise
// the public APIs.
class SimpleRemotingReceiver {
 public:
  explicit SimpleRemotingReceiver(RpcMessenger* messenger);
  ~SimpleRemotingReceiver();

  // The flow here closely mirrors the remoting.proto. The standalone receiver
  // indicates it is ready for initialization by calling
  // |SendInitializeMessage|, then this class sends an initialize message to the
  // sender. The sender then replies with an initialization message containing
  // configurations, which is passed to |initialize_cb|.
  using InitializeCallback = std::function<void(AudioCodec, VideoCodec)>;
  void SendInitializeMessage(InitializeCallback initialize_cb);

  // The speed at which the content is decoded is synchronized with the
  // playback rate. Pausing is a special case with a playback rate of 0.0.
  void SendPlaybackRateMessage(double playback_rate);

 private:
  void OnInitializeCallbackMessage(std::unique_ptr<RpcMessage> message);

  RpcMessenger* messenger_;
  InitializeCallback initialize_cb_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_RECEIVER_SIMPLE_REMOTING_RECEIVER_H_
