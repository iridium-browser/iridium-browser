// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_receiver.h"

#include <utility>

#include "discovery/mdns/mdns_reader.h"
#include "util/std_util.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace discovery {

MdnsReceiver::ResponseClient::~ResponseClient() = default;

MdnsReceiver::MdnsReceiver(Config config) : config_(std::move(config)) {}

MdnsReceiver::~MdnsReceiver() {
  if (state_ == State::kRunning) {
    Stop();
  }

  OSP_DCHECK(response_clients_.empty());
}

void MdnsReceiver::SetQueryCallback(
    std::function<void(const MdnsMessage&, const IPEndpoint&)> callback) {
  // This check verifies that either new or stored callback has a target. It
  // will fail in case multiple objects try to set or clear the callback.
  OSP_DCHECK(static_cast<bool>(query_callback_) != static_cast<bool>(callback));
  query_callback_ = callback;
}

void MdnsReceiver::AddResponseCallback(ResponseClient* callback) {
  OSP_DCHECK(!Contains(response_clients_, callback));
  response_clients_.push_back(callback);
}

void MdnsReceiver::RemoveResponseCallback(ResponseClient* callback) {
  auto it =
      std::find(response_clients_.begin(), response_clients_.end(), callback);
  OSP_DCHECK(it != response_clients_.end());

  response_clients_.erase(it);
}

void MdnsReceiver::Start() {
  state_ = State::kRunning;
}

void MdnsReceiver::Stop() {
  state_ = State::kStopped;
}

void MdnsReceiver::OnRead(UdpSocket* socket,
                          ErrorOr<UdpPacket> packet_or_error) {
  if (state_ != State::kRunning || packet_or_error.is_error()) {
    return;
  }

  UdpPacket packet = std::move(packet_or_error.value());

  TRACE_SCOPED(TraceCategory::kMdns, "MdnsReceiver::OnRead");
  MdnsReader reader(config_, packet.data(), packet.size());
  const ErrorOr<MdnsMessage> message = reader.Read();
  if (message.is_error()) {
    if (message.error().code() == Error::Code::kMdnsNonConformingFailure) {
      OSP_DVLOG << "mDNS message dropped due to invalid rcode or opcode...";
    } else {
      OSP_DVLOG << "mDNS message failed to parse...";
    }
    return;
  }

  if (message.value().type() == MessageType::Response) {
    for (ResponseClient* client : response_clients_) {
      client->OnMessageReceived(message.value());
    }
    if (response_clients_.empty()) {
      OSP_DVLOG
          << "mDNS response message dropped. No response client registered...";
    }
  } else {
    if (query_callback_) {
      query_callback_(message.value(), packet.source());
    } else {
      OSP_DVLOG << "mDNS query message dropped. No query client registered...";
    }
  }
}

}  // namespace discovery
}  // namespace openscreen
