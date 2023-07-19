// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CHANNEL_CONNECTION_NAMESPACE_HANDLER_H_
#define CAST_COMMON_CHANNEL_CONNECTION_NAMESPACE_HANDLER_H_

#include <functional>
#include <vector>

#include "cast/common/channel/cast_message_handler.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/channel/virtual_connection.h"
#include "util/json/json_serialization.h"

namespace openscreen {
namespace cast {

class VirtualConnectionRouter;

// Handles CastMessages in the connection namespace by opening and closing
// VirtualConnections on the socket on which the messages were received.
//
// This is meant to be used in either/both the initiator or responder role:
//
// 1. Initiators call Open/CloseRemoteConnection() to establish/close a virtual
// connection with a remote peer. Internally, OpenRemoteConnection() sends a
// CONNECT request to the remote peer, and the remote peer is expected to
// respond with a either a CONNECTED response or a CLOSE response.
//
// 2. Responders simply handle CONNECT or CLOSE requests, allowing or
// disallowing connections based on the VirtualConnectionPolicy, and
// ConnectionNamespaceHandler will report new/closed connections to the local
// VirtualConnectionRouter to enable/disable message routing.
class ConnectionNamespaceHandler : public CastMessageHandler {
 public:
  class VirtualConnectionPolicy {
   public:
    virtual ~VirtualConnectionPolicy() = default;

    virtual bool IsConnectionAllowed(
        const VirtualConnection& virtual_conn) const = 0;
  };

  using RemoteConnectionResultCallback = std::function<void(bool)>;

  // Both |vc_router| and |vc_policy| should outlive this object.
  ConnectionNamespaceHandler(VirtualConnectionRouter* vc_router,
                             VirtualConnectionPolicy* vc_policy);
  ~ConnectionNamespaceHandler() override;

  // Requests a virtual connection be established. The |result_callback| is
  // later invoked with true/false to indicate success, based on a response from
  // the remote.
  void OpenRemoteConnection(VirtualConnection conn,
                            RemoteConnectionResultCallback result_callback);

  // Closes the virtual connection, notifying the remote by sending it a CLOSE
  // message.
  void CloseRemoteConnection(VirtualConnection conn);

  // CastMessageHandler overrides.
  void OnMessage(VirtualConnectionRouter* router,
                 CastSocket* socket,
                 ::cast::channel::CastMessage message) override;

 private:
  void HandleConnect(CastSocket* socket,
                     ::cast::channel::CastMessage message,
                     Json::Value parsed_message);
  void HandleClose(CastSocket* socket,
                   ::cast::channel::CastMessage message,
                   Json::Value parsed_message);
  void HandleConnectedResponse(CastSocket* socket,
                               ::cast::channel::CastMessage message,
                               Json::Value parsed_message);

  void SendConnect(VirtualConnection virtual_conn);
  void SendClose(VirtualConnection virtual_conn);
  void SendConnectedResponse(const VirtualConnection& virtual_conn,
                             int max_protocol_version);

  bool RemoveConnection(const VirtualConnection& conn,
                        VirtualConnection::CloseReason reason);

  VirtualConnectionRouter* const vc_router_;
  VirtualConnectionPolicy* const vc_policy_;

  struct PendingRequest {
    VirtualConnection conn;
    RemoteConnectionResultCallback result_callback;
  };
  std::vector<PendingRequest> pending_remote_requests_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CHANNEL_CONNECTION_NAMESPACE_HANDLER_H_
