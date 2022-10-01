// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_TLS_CONNECTION_POSIX_H_
#define PLATFORM_IMPL_TLS_CONNECTION_POSIX_H_

#include <openssl/ssl.h>

#include <memory>

#include "platform/api/tls_connection.h"
#include "platform/impl/platform_client_posix.h"
#include "platform/impl/stream_socket_posix.h"
#include "platform/impl/tls_write_buffer.h"
#include "util/weak_ptr.h"

namespace openscreen {

class TaskRunner;
class TlsConnectionFactoryPosix;

class TlsConnectionPosix : public TlsConnection {
 public:
  ~TlsConnectionPosix() override;

  // Sends any available bytes from this connection's buffer_.
  virtual void SendAvailableBytes();

  // Read out a block/message, if one is available, and notify this instance's
  // TlsConnection::Client.
  virtual void TryReceiveMessage();

  // TlsConnection overrides.
  void SetClient(Client* client) override;
  bool Send(const void* data, size_t len) override;
  IPEndpoint GetRemoteEndpoint() const override;

  // Registers |this| with the platform TlsDataRouterPosix.  This is called
  // automatically by TlsConnectionFactoryPosix after the handshake completes.
  void RegisterConnectionWithDataRouter(PlatformClientPosix* platform_client);

  const SocketHandle& socket_handle() const { return socket_->socket_handle(); }

 protected:
  friend class TlsConnectionFactoryPosix;

  TlsConnectionPosix(IPEndpoint local_address, TaskRunner* task_runner);
  TlsConnectionPosix(IPAddress::Version version, TaskRunner* task_runner);
  TlsConnectionPosix(std::unique_ptr<StreamSocket> socket,
                     TaskRunner* task_runner);

 private:
  // Called on any thread, to post a task to notify the Client that an |error|
  // has occurred.
  void DispatchError(Error error);

  TaskRunner* const task_runner_;
  PlatformClientPosix* platform_client_ = nullptr;

  Client* client_ = nullptr;

  std::unique_ptr<StreamSocket> socket_;
  bssl::UniquePtr<SSL> ssl_;

  TlsWriteBuffer buffer_;

  WeakPtrFactory<TlsConnectionPosix> weak_factory_{this};

  OSP_DISALLOW_COPY_AND_ASSIGN(TlsConnectionPosix);
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_TLS_CONNECTION_POSIX_H_
