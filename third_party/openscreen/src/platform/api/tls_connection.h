// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TLS_CONNECTION_H_
#define PLATFORM_API_TLS_CONNECTION_H_

#include <cstdint>
#include <vector>

#include "platform/base/error.h"
#include "platform/base/ip_address.h"

namespace openscreen {

class TlsConnection {
 public:
  // Client callbacks are run via the TaskRunner used by TlsConnectionFactory.
  class Client {
   public:
    // Called when |connection| experiences an error, such as a read error.
    virtual void OnError(TlsConnection* connection, Error error) = 0;

    // Called when a |block| arrives on |connection|.
    virtual void OnRead(TlsConnection* connection,
                        std::vector<uint8_t> block) = 0;

   protected:
    virtual ~Client();
  };

  virtual ~TlsConnection();

  // Sets the Client associated with this instance. This should be called as
  // soon as the factory provides a new TlsConnection instance via
  // TlsConnectionFactory::OnAccepted() or OnConnected(). Pass nullptr to unset
  // the Client.
  virtual void SetClient(Client* client) = 0;

  // Sends a message. Returns true iff the message will be sent.
  [[nodiscard]] virtual bool Send(const void* data, size_t len) = 0;

  // Get the connected remote address.
  virtual IPEndpoint GetRemoteEndpoint() const = 0;

 protected:
  TlsConnection();
};

}  // namespace openscreen

#endif  // PLATFORM_API_TLS_CONNECTION_H_
