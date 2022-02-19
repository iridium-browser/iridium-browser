// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_sender.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "discovery/mdns/mdns_writer.h"
#include "platform/api/udp_socket.h"

namespace openscreen {
namespace discovery {

MdnsSender::MdnsSender(UdpSocket* socket) : socket_(socket) {
  OSP_DCHECK(socket_ != nullptr);
}

MdnsSender::~MdnsSender() = default;

Error MdnsSender::SendMulticast(const MdnsMessage& message) {
  const IPEndpoint& endpoint = socket_->IsIPv6() ? kMulticastSendIPv6Endpoint
                                                 : kMulticastSendIPv4Endpoint;
  return SendMessage(message, endpoint);
}

Error MdnsSender::SendMessage(const MdnsMessage& message,
                              const IPEndpoint& endpoint) {
  // Always try to write the message into the buffer even if MaxWireSize is
  // greater than maximum message size. Domain name compression might reduce the
  // on-the-wire size of the message sufficiently for it to fit into the buffer.
  std::vector<uint8_t> buffer(
      std::min(message.MaxWireSize(), kMaxMulticastMessageSize));
  MdnsWriter writer(buffer.data(), buffer.size());
  if (!writer.Write(message)) {
    return Error::Code::kInsufficientBuffer;
  }

  socket_->SendMessage(buffer.data(), writer.offset(), endpoint);
  return Error::Code::kNone;
}

void MdnsSender::OnSendError(UdpSocket* socket, Error error) {
  OSP_LOG_ERROR << "Error sending packet";
}

}  // namespace discovery
}  // namespace openscreen
