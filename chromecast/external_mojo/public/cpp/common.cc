// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/external_mojo/public/cpp/common.h"

#include "base/command_line.h"
#include "base/strings/string_piece.h"

namespace chromecast {
namespace external_mojo {

namespace {
// Default path for Unix domain socket used by external Mojo services to connect
// to Mojo services within cast_shell.
constexpr base::StringPiece kDefaultBrokerPath("/tmp/cast_mojo_broker");

// Command-line arg to change the Unix domain socket path to connect to the
// Cast Mojo broker.
constexpr base::StringPiece kCastMojoBrokerPathSwitch("cast-mojo-broker-path");
}  // namespace

std::string GetBrokerPath() {
  std::string broker_path;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kCastMojoBrokerPathSwitch)) {
    broker_path = command_line->GetSwitchValueASCII(kCastMojoBrokerPathSwitch);
  } else {
    broker_path = std::string(kDefaultBrokerPath);
  }
  return broker_path;
}

}  // namespace external_mojo
}  // namespace chromecast
