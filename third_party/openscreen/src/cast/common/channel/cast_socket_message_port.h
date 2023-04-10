// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CHANNEL_CAST_SOCKET_MESSAGE_PORT_H_
#define CAST_COMMON_CHANNEL_CAST_SOCKET_MESSAGE_PORT_H_

#include <memory>
#include <string>
#include <vector>

#include "cast/common/channel/cast_message_handler.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/common/public/message_port.h"
#include "util/weak_ptr.h"

namespace openscreen {
namespace cast {

class CastSocketMessagePort : public MessagePort, public CastMessageHandler {
 public:
  // The router is expected to outlive this message port.
  explicit CastSocketMessagePort(VirtualConnectionRouter* router);
  ~CastSocketMessagePort() override;

  const std::string& source_id() const { return source_id_; }

  void SetSocket(WeakPtr<CastSocket> socket);

  // Returns current socket identifier, or ToCastSocketId(nullptr) if not
  // connected.
  int GetSocketId();

  // MessagePort overrides.
  void SetClient(MessagePort::Client& client) override;
  void ResetClient() override;
  void PostMessage(const std::string& destination_sender_id,
                   const std::string& message_namespace,
                   const std::string& message) override;

  // CastMessageHandler overrides.
  void OnMessage(VirtualConnectionRouter* router,
                 CastSocket* socket,
                 ::cast::channel::CastMessage message) override;

 private:
  VirtualConnectionRouter* const router_;
  std::string source_id_;
  MessagePort::Client* client_ = nullptr;
  WeakPtr<CastSocket> socket_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CHANNEL_CAST_SOCKET_MESSAGE_PORT_H_
