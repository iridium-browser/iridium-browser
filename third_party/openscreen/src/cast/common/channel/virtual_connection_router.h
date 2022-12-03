// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CHANNEL_VIRTUAL_CONNECTION_ROUTER_H_
#define CAST_COMMON_CHANNEL_VIRTUAL_CONNECTION_ROUTER_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "absl/types/optional.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/channel/virtual_connection.h"
#include "cast/common/public/cast_socket.h"

namespace openscreen {
namespace cast {

class CastMessageHandler;
class ConnectionNamespaceHandler;

// Handles CastSockets by routing received messages to appropriate message
// handlers based on the VirtualConnection's local ID and sending messages over
// the appropriate CastSocket for a VirtualConnection.
//
// Basic model for using this would be:
//
// 1. Foo is a SenderSocketFactory::Client.
//
// 2. Foo calls SenderSocketFactory::Connect, optionally with VCRouter as the
//    CastSocket::Client.
//
// 3. Foo gets OnConnected callback and makes whatever local notes it needs
//    (e.g.  sink "resolved", init app probing state, etc.), then calls
//    VCRouter::TakeSocket.
//
// 4. Anything Foo wants to send (launch, app availability, etc.) goes through
//    VCRouter::Send via an appropriate VC.  The virtual connection is not
//    created automatically, so AddConnection() must be called first.
//
// 5. Anything Foo wants to receive must be registered with a handler by calling
//    AddHandlerForLocalId().
//
// 6. Foo is expected to clean-up after itself (#4 and #5) by calling
//    RemoveConnection() and RemoveHandlerForLocalId().
class VirtualConnectionRouter final : public CastSocket::Client {
 public:
  class SocketErrorHandler {
   public:
    virtual void OnClose(CastSocket* socket) = 0;
    virtual void OnError(CastSocket* socket, Error error) = 0;

   protected:
    virtual ~SocketErrorHandler();
  };

  VirtualConnectionRouter();
  ~VirtualConnectionRouter() override;

  // Adds a VirtualConnection, if one does not already exist, to enable routing
  // of peer-to-peer messages.
  void AddConnection(VirtualConnection virtual_connection,
                     VirtualConnection::AssociatedData associated_data);

  // Removes a VirtualConnection and returns true if a connection matching
  // |virtual_connection| was found and removed.
  bool RemoveConnection(const VirtualConnection& virtual_connection,
                        VirtualConnection::CloseReason reason);

  // Removes all VirtualConnections whose local endpoint matches the given
  // |local_id|.
  void RemoveConnectionsByLocalId(const std::string& local_id);

  // Removes all VirtualConnections whose traffic passes over the socket
  // referenced by |socket_id|.
  void RemoveConnectionsBySocketId(int socket_id);

  // Returns the AssociatedData for a |virtual_connection| if a connection
  // exists, nullopt otherwise. The pointer isn't stable in the long term; so,
  // if it actually needs to be stored for later, the caller should make a copy.
  absl::optional<const VirtualConnection::AssociatedData*> GetConnectionData(
      const VirtualConnection& virtual_connection) const;

  // Adds/Removes a CastMessageHandler for all messages destined for the given
  // |endpoint| referred to by |local_id|, and returns whether the given
  // |local_id| was successfully added/removed.
  //
  // Note: Clients will need to separately call AddConnection(), and
  // RemoveConnection() or RemoveConnectionsByLocalId().
  bool AddHandlerForLocalId(std::string local_id, CastMessageHandler* endpoint);
  bool RemoveHandlerForLocalId(const std::string& local_id);

  // |error_handler| must live until either its OnError or OnClose is called.
  void TakeSocket(SocketErrorHandler* error_handler,
                  std::unique_ptr<CastSocket> socket);
  void CloseSocket(int id);

  Error Send(VirtualConnection virtual_conn,
             ::cast::channel::CastMessage message);

  Error BroadcastFromLocalPeer(std::string local_id,
                               ::cast::channel::CastMessage message);

  // CastSocket::Client overrides.
  void OnError(CastSocket* socket, Error error) override;
  void OnMessage(CastSocket* socket,
                 ::cast::channel::CastMessage message) override;

 protected:
  friend class ConnectionNamespaceHandler;

  void set_connection_namespace_handler(ConnectionNamespaceHandler* handler) {
    connection_handler_ = handler;
  }

 private:
  // This struct simply stores the remainder of the data {VirtualConnection,
  // VirtualConnection::AssociatedData} that is not broken up into map keys for
  // |connections_|.
  struct VCTail {
    std::string peer_id;
    VirtualConnection::AssociatedData data;
  };

  struct SocketWithHandler {
    std::unique_ptr<CastSocket> socket;
    SocketErrorHandler* error_handler;
  };

  ConnectionNamespaceHandler* connection_handler_ = nullptr;

  std::map<int /* socket_id */,
           std::multimap<std::string /* local_id */, VCTail>>
      connections_;

  std::map<int, SocketWithHandler> sockets_;
  std::map<std::string /* local_id */, CastMessageHandler*> endpoints_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CHANNEL_VIRTUAL_CONNECTION_ROUTER_H_
