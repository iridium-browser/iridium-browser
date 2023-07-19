// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_SENDER_PUBLIC_SENDER_SOCKET_FACTORY_H_
#define CAST_SENDER_PUBLIC_SENDER_SOCKET_FACTORY_H_

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "cast/common/public/cast_socket.h"
#include "cast/common/public/parsed_certificate.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/api/task_runner.h"
#include "platform/api/tls_connection_factory.h"
#include "platform/base/ip_address.h"

namespace openscreen {
namespace cast {

class AuthContext;
class TrustStore;

class SenderSocketFactory final : public TlsConnectionFactory::Client,
                                  public CastSocket::Client {
 public:
  class Client {
   public:
    virtual void OnConnected(SenderSocketFactory* factory,
                             const IPEndpoint& endpoint,
                             std::unique_ptr<CastSocket> socket) = 0;
    virtual void OnError(SenderSocketFactory* factory,
                         const IPEndpoint& endpoint,
                         Error error) = 0;

   protected:
    virtual ~Client();
  };

  enum class DeviceMediaPolicy {
    kNone = 0,
    kAudioOnly,
    kIncludesVideo,
  };

  // |client| and |task_runner| must outlive |this|.  If no trust stores are
  // passed, the default production certificates are used.
  SenderSocketFactory(Client* client, TaskRunner* task_runner);
  SenderSocketFactory(Client* client,
                      TaskRunner* task_runner,
                      std::unique_ptr<TrustStore> cast_trust_store,
                      std::unique_ptr<TrustStore> crl_trust_store);
  ~SenderSocketFactory();

  // |factory| cannot be nullptr and must outlive |this|.
  void set_factory(TlsConnectionFactory* factory);

  // Begins connecting to a Cast device at |endpoint|.  If a successful
  // connection is made, including device authentication, the new CastSocket
  // will be passed to |client_|'s OnConnected method.  The new CastSocket will
  // have its client set to |client|.  If any part of the connection process
  // fails, |client_|'s OnError method is called instead.  This includes if the
  // device's media policy, as determined by authentication, is audio-only and
  // |media_policy| is kIncludesVideo.
  void Connect(const IPEndpoint& endpoint,
               DeviceMediaPolicy media_policy,
               CastSocket::Client* client);

  // TlsConnectionFactory::Client overrides.
  void OnAccepted(TlsConnectionFactory* factory,
                  std::vector<uint8_t> der_x509_peer_cert,
                  std::unique_ptr<TlsConnection> connection) override;
  void OnConnected(TlsConnectionFactory* factory,
                   std::vector<uint8_t> der_x509_peer_cert,
                   std::unique_ptr<TlsConnection> connection) override;
  void OnConnectionFailed(TlsConnectionFactory* factory,
                          const IPEndpoint& remote_address) override;
  void OnError(TlsConnectionFactory* factory, Error error) override;

 private:
  struct PendingConnection {
    IPEndpoint endpoint;
    DeviceMediaPolicy media_policy;
    CastSocket::Client* client;
  };

  struct PendingAuth {
    IPEndpoint endpoint;
    DeviceMediaPolicy media_policy;
    SerialDeletePtr<CastSocket> socket;
    CastSocket::Client* client;
    std::unique_ptr<AuthContext> auth_context;
    std::unique_ptr<ParsedCertificate> peer_cert;
  };

  friend bool operator<(const std::unique_ptr<PendingAuth>& a, int b);
  friend bool operator<(int a, const std::unique_ptr<PendingAuth>& b);

  std::vector<PendingConnection>::iterator FindPendingConnection(
      const IPEndpoint& endpoint);

  // CastSocket::Client overrides.
  void OnError(CastSocket* socket, Error error) override;
  void OnMessage(CastSocket* socket,
                 ::cast::channel::CastMessage message) override;

  Client* const client_;
  TaskRunner* const task_runner_;
  TlsConnectionFactory* factory_ = nullptr;
  std::vector<PendingConnection> pending_connections_;
  std::vector<std::unique_ptr<PendingAuth>> pending_auth_;

  // Trust stores for use with AuthenticateChallengeReply.
  std::unique_ptr<TrustStore> cast_trust_store_;
  std::unique_ptr<TrustStore> crl_trust_store_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_SENDER_PUBLIC_SENDER_SOCKET_FACTORY_H_
