// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_CONNECTION_SETTINGS_H_
#define CAST_STANDALONE_SENDER_CONNECTION_SETTINGS_H_

#include <string>

#include "cast/streaming/constants.h"
#include "platform/base/interface_info.h"

namespace openscreen {
namespace cast {

// The connection settings for a given standalone sender instance. These fields
// are used throughout the standalone sender component to initialize state from
// the command line parameters.
struct ConnectionSettings {
  // The endpoint of the receiver we wish to connect to.
  IPEndpoint receiver_endpoint;

  // The path to the file that we want to play.
  std::string path_to_file;

  // The maximum bitrate. Default value means a reasonable default will be
  // selected.
  int max_bitrate = 0;

  // Whether the stream should include video, or just be audio only.
  bool should_include_video = true;

  // Whether we should use the hacky RTP stream IDs for legacy android
  // receivers, or if we should use the proper values. For more information,
  // see https://issuetracker.google.com/184438154.
  bool use_android_rtp_hack = true;

  // Whether we should use remoting for the video, instead of the default of
  // mirroring.
  bool use_remoting = false;

  // Whether we should loop the video when it is completed.
  bool should_loop_video = true;

  // The codec to use for encoding negotiated video streams.
  VideoCodec codec;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_CONNECTION_SETTINGS_H_
