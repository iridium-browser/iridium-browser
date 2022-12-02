// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/channel/cast_socket_message_port.h"

#include <utility>

#include "cast/common/channel/message_util.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/channel/virtual_connection.h"

namespace openscreen {
namespace cast {

CastSocketMessagePort::CastSocketMessagePort(VirtualConnectionRouter* router)
    : router_(router) {}

CastSocketMessagePort::~CastSocketMessagePort() {
  ResetClient();
}

// NOTE: we assume here that this message port is already the client for
// the passed in socket, so leave the socket's client unchanged. However,
// since sockets should map one to one with receiver sessions, we reset our
// client. The consumer of this message port should call SetClient with the new
// message port client after setting the socket.
void CastSocketMessagePort::SetSocket(WeakPtr<CastSocket> socket) {
  ResetClient();
  socket_ = socket;
}

int CastSocketMessagePort::GetSocketId() {
  return ToCastSocketId(socket_.get());
}

void CastSocketMessagePort::SetClient(MessagePort::Client& client) {
  ResetClient();

  client_ = &client;
  source_id_ = client.source_id();
  router_->AddHandlerForLocalId(source_id_, this);
}

void CastSocketMessagePort::ResetClient() {
  if (!client_) {
    return;
  }

  client_ = nullptr;
  router_->RemoveHandlerForLocalId(source_id_);
  router_->RemoveConnectionsByLocalId(source_id_);
  source_id_.clear();
}

void CastSocketMessagePort::PostMessage(
    const std::string& destination_sender_id,
    const std::string& message_namespace,
    const std::string& message) {
  if (!client_) {
    OSP_DLOG_WARN << "Not posting message due to nullptr client_";
    return;
  }

  if (!socket_) {
    client_->OnError(Error::Code::kAlreadyClosed);
    return;
  }

  VirtualConnection connection{source_id_, destination_sender_id,
                               socket_->socket_id()};
  if (!router_->GetConnectionData(connection)) {
    router_->AddConnection(connection, VirtualConnection::AssociatedData{});
  }

  const Error send_error = router_->Send(
      std::move(connection), MakeSimpleUTF8Message(message_namespace, message));
  if (!send_error.ok()) {
    client_->OnError(std::move(send_error));
  }
}

void CastSocketMessagePort::OnMessage(VirtualConnectionRouter* router,
                                      CastSocket* socket,
                                      ::cast::channel::CastMessage message) {
  OSP_DCHECK(router == router_);
  OSP_DCHECK(!socket || socket_.get() == socket);

  // Message ports are for specific virtual connections, and do not pass-through
  // broadcasts.
  if (message.destination_id() == kBroadcastId) {
    return;
  }

  if (!client_) {
    OSP_DLOG_WARN << "Dropping message due to nullptr client_";
    return;
  }

  client_->OnMessage(message.source_id(), message.namespace_(),
                     message.payload_utf8());
}

}  // namespace cast
}  // namespace openscreen
