// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TLS_CONNECTION_FACTORY_H_
#define PLATFORM_API_TLS_CONNECTION_FACTORY_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "platform/base/ip_address.h"

namespace openscreen {

class TaskRunner;
class TlsConnection;
struct TlsConnectOptions;
struct TlsCredentials;
struct TlsListenOptions;

// We expect a single factory to be able to handle an arbitrary number of
// calls using the same client and task runner.
class TlsConnectionFactory {
 public:
  // Client callbacks are ran on the provided TaskRunner.
  class Client {
   public:
    // Provides a new |connection| that resulted from listening on the local
    // socket. |der_x509_peer_cert| is the DER-encoded X509 certificate from the
    // peer if present, or empty if the peer didn't provide one.
    virtual void OnAccepted(TlsConnectionFactory* factory,
                            std::vector<uint8_t> der_x509_peer_cert,
                            std::unique_ptr<TlsConnection> connection) = 0;

    // Provides a new |connection| that resulted from connecting to a remote
    // endpoint. |der_x509_peer_cert| is the DER-encoded X509 certificate from
    // the peer.
    virtual void OnConnected(TlsConnectionFactory* factory,
                             std::vector<uint8_t> der_x509_peer_cert,
                             std::unique_ptr<TlsConnection> connection) = 0;

    virtual void OnConnectionFailed(TlsConnectionFactory* factory,
                                    const IPEndpoint& remote_address) = 0;

    // Called when a non-recoverable error occurs.
    virtual void OnError(TlsConnectionFactory* factory, Error error) = 0;

   protected:
    virtual ~Client();
  };

  // The connection factory requires a client for yielding creation results
  // asynchronously, as well as a task runner it can use to for running
  // callbacks both on the factory and on created TlsConnection instances.
  static std::unique_ptr<TlsConnectionFactory> CreateFactory(
      Client* client,
      TaskRunner& task_runner);

  virtual ~TlsConnectionFactory();

  // Fires an OnConnected or OnConnectionFailed event.
  virtual void Connect(const IPEndpoint& remote_address,
                       const TlsConnectOptions& options) = 0;

  // Set the TlsCredentials used for listening for new connections. Currently,
  // having different certificates on different address is not supported. This
  // must be called before the first call to Listen.
  virtual void SetListenCredentials(const TlsCredentials& credentials) = 0;

  // Fires an OnAccepted or OnConnectionFailed event.
  virtual void Listen(const IPEndpoint& local_address,
                      const TlsListenOptions& options) = 0;

 protected:
  TlsConnectionFactory();
};

}  // namespace openscreen

#endif  // PLATFORM_API_TLS_CONNECTION_FACTORY_H_
