// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/udp_socket.h"

namespace openscreen {

UdpSocket::UdpSocket() = default;
UdpSocket::~UdpSocket() = default;

UdpSocket::Client::~Client() = default;

}  // namespace openscreen
