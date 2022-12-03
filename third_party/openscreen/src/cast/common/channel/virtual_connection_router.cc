// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/channel/virtual_connection_router.h"

#include <utility>

#include "cast/common/channel/cast_message_handler.h"
#include "cast/common/channel/connection_namespace_handler.h"
#include "cast/common/channel/message_util.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

using ::cast::channel::CastMessage;

VirtualConnectionRouter::SocketErrorHandler::~SocketErrorHandler() = default;

VirtualConnectionRouter::VirtualConnectionRouter() = default;

VirtualConnectionRouter::~VirtualConnectionRouter() = default;

void VirtualConnectionRouter::AddConnection(
    VirtualConnection virtual_connection,
    VirtualConnection::AssociatedData associated_data) {
  auto& socket_map = connections_[virtual_connection.socket_id];
  auto local_entries = socket_map.equal_range(virtual_connection.local_id);
  auto it = std::find_if(
      local_entries.first, local_entries.second,
      [&virtual_connection](const std::pair<std::string, VCTail>& entry) {
        return entry.second.peer_id == virtual_connection.peer_id;
      });
  if (it == socket_map.end()) {
    socket_map.emplace(std::move(virtual_connection.local_id),
                       VCTail{std::move(virtual_connection.peer_id),
                              std::move(associated_data)});
  }
}

bool VirtualConnectionRouter::RemoveConnection(
    const VirtualConnection& virtual_connection,
    VirtualConnection::CloseReason reason) {
  auto socket_entry = connections_.find(virtual_connection.socket_id);
  if (socket_entry == connections_.end()) {
    return false;
  }

  auto& socket_map = socket_entry->second;
  auto local_entries = socket_map.equal_range(virtual_connection.local_id);
  if (local_entries.first == socket_map.end()) {
    return false;
  }
  for (auto it = local_entries.first; it != local_entries.second; ++it) {
    if (it->second.peer_id == virtual_connection.peer_id) {
      socket_map.erase(it);
      if (socket_map.empty()) {
        connections_.erase(socket_entry);
      }
      return true;
    }
  }
  return false;
}

void VirtualConnectionRouter::RemoveConnectionsByLocalId(
    const std::string& local_id) {
  for (auto socket_entry = connections_.begin();
       socket_entry != connections_.end();) {
    auto& socket_map = socket_entry->second;
    auto local_entries = socket_map.equal_range(local_id);
    if (local_entries.first != socket_map.end()) {
      socket_map.erase(local_entries.first, local_entries.second);
      if (socket_map.empty()) {
        socket_entry = connections_.erase(socket_entry);
        continue;
      }
    }
    ++socket_entry;
  }
}

void VirtualConnectionRouter::RemoveConnectionsBySocketId(int socket_id) {
  auto entry = connections_.find(socket_id);
  if (entry != connections_.end()) {
    connections_.erase(entry);
  }
}

absl::optional<const VirtualConnection::AssociatedData*>
VirtualConnectionRouter::GetConnectionData(
    const VirtualConnection& virtual_connection) const {
  auto socket_entry = connections_.find(virtual_connection.socket_id);
  if (socket_entry == connections_.end()) {
    return absl::nullopt;
  }

  auto& socket_map = socket_entry->second;
  auto local_entries = socket_map.equal_range(virtual_connection.local_id);
  if (local_entries.first == socket_map.end()) {
    return absl::nullopt;
  }
  for (auto it = local_entries.first; it != local_entries.second; ++it) {
    if (it->second.peer_id == virtual_connection.peer_id) {
      return &it->second.data;
    }
  }
  return absl::nullopt;
}

bool VirtualConnectionRouter::AddHandlerForLocalId(
    std::string local_id,
    CastMessageHandler* endpoint) {
  return endpoints_.emplace(std::move(local_id), endpoint).second;
}

bool VirtualConnectionRouter::RemoveHandlerForLocalId(
    const std::string& local_id) {
  return endpoints_.erase(local_id) == 1u;
}

void VirtualConnectionRouter::TakeSocket(SocketErrorHandler* error_handler,
                                         std::unique_ptr<CastSocket> socket) {
  int id = socket->socket_id();
  socket->SetClient(this);
  sockets_.emplace(id, SocketWithHandler{std::move(socket), error_handler});
}

void VirtualConnectionRouter::CloseSocket(int id) {
  auto it = sockets_.find(id);
  if (it != sockets_.end()) {
    RemoveConnectionsBySocketId(id);
    std::unique_ptr<CastSocket> socket = std::move(it->second.socket);
    SocketErrorHandler* error_handler = it->second.error_handler;
    sockets_.erase(it);
    error_handler->OnClose(socket.get());
  }
}

Error VirtualConnectionRouter::Send(VirtualConnection virtual_conn,
                                    CastMessage message) {
  if (virtual_conn.peer_id == kBroadcastId) {
    return BroadcastFromLocalPeer(std::move(virtual_conn.local_id),
                                  std::move(message));
  }

  if (!IsTransportNamespace(message.namespace_()) &&
      !GetConnectionData(virtual_conn)) {
    return Error::Code::kNoActiveConnection;
  }
  auto it = sockets_.find(virtual_conn.socket_id);
  if (it == sockets_.end()) {
    return Error::Code::kItemNotFound;
  }
  message.set_source_id(std::move(virtual_conn.local_id));
  message.set_destination_id(std::move(virtual_conn.peer_id));
  return it->second.socket->Send(message);
}

Error VirtualConnectionRouter::BroadcastFromLocalPeer(
    std::string local_id,
    ::cast::channel::CastMessage message) {
  message.set_source_id(std::move(local_id));
  message.set_destination_id(kBroadcastId);

  // Broadcast to local endpoints.
  for (const auto& entry : endpoints_) {
    if (entry.first != message.source_id()) {
      entry.second->OnMessage(this, nullptr, message);
    }
  }

  // Broadcast to remote endpoints. If an Error occurs, continue broadcasting,
  // and later return the first Error that occurred.
  Error error;
  for (const auto& entry : sockets_) {
    auto result = entry.second.socket->Send(message);
    if (!result.ok() && error.ok()) {
      error = std::move(result);
    }
  }
  return error;
}

void VirtualConnectionRouter::OnError(CastSocket* socket, Error error) {
  const int id = socket->socket_id();
  auto it = sockets_.find(id);
  if (it != sockets_.end()) {
    RemoveConnectionsBySocketId(id);
    std::unique_ptr<CastSocket> socket_owned = std::move(it->second.socket);
    SocketErrorHandler* error_handler = it->second.error_handler;
    sockets_.erase(it);
    error_handler->OnError(socket, error);
  }
}

void VirtualConnectionRouter::OnMessage(CastSocket* socket,
                                        CastMessage message) {
  OSP_DCHECK(socket);

  const std::string& local_id = message.destination_id();
  if (local_id == kBroadcastId) {
    for (const auto& entry : endpoints_) {
      entry.second->OnMessage(this, socket, message);
    }
  } else {
    // Connection namespace messages are weird: The message.source_id() and
    // message.destination_id() are NOT treated as "envelope routing
    // information," like for all other namespaces. Instead, they are considered
    // part of the payload data for CONNECT/CLOSE requests. Thus, they require
    // special-case handling here.
    if (message.namespace_() == kConnectionNamespace) {
      if (connection_handler_) {
        connection_handler_->OnMessage(this, socket, std::move(message));
      }
      return;
    }

    // Drop all messages for virtual connections that do not yet exist.
    // Exception: All transport namespace messages (e.g., device auth,
    // heartbeats, etc.); because these are always assumed to have a route.
    if (!IsTransportNamespace(message.namespace_()) &&
        !GetConnectionData(VirtualConnection{local_id, message.source_id(),
                                             socket->socket_id()})) {
      return;
    }
    auto it = endpoints_.find(local_id);
    if (it != endpoints_.end()) {
      it->second->OnMessage(this, socket, std::move(message));
    }
  }
}

}  // namespace cast
}  // namespace openscreen
