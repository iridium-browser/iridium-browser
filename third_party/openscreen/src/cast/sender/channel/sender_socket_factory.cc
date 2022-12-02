// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/public/sender_socket_factory.h"

#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/public/trust_store.h"
#include "cast/sender/channel/cast_auth_util.h"
#include "cast/sender/channel/message_util.h"
#include "platform/base/tls_connect_options.h"
#include "util/crypto/certificate_utils.h"
#include "util/osp_logging.h"

using ::cast::channel::CastMessage;

namespace openscreen {
namespace cast {

SenderSocketFactory::Client::~Client() = default;

bool operator<(const std::unique_ptr<SenderSocketFactory::PendingAuth>& a,
               int b) {
  return a && a->socket->socket_id() < b;
}

bool operator<(int a,
               const std::unique_ptr<SenderSocketFactory::PendingAuth>& b) {
  return b && a < b->socket->socket_id();
}

SenderSocketFactory::SenderSocketFactory(Client* client,
                                         TaskRunner* task_runner)
    : SenderSocketFactory(client,
                          task_runner,
                          CastTrustStore::Create(),
                          CastCRLTrustStore::Create()) {}

SenderSocketFactory::SenderSocketFactory(
    Client* client,
    TaskRunner* task_runner,
    std::unique_ptr<TrustStore> cast_trust_store,
    std::unique_ptr<TrustStore> crl_trust_store)
    : client_(client),
      task_runner_(task_runner),
      cast_trust_store_(std::move(cast_trust_store)),
      crl_trust_store_(std::move(crl_trust_store)) {
  OSP_DCHECK(client);
  OSP_DCHECK(task_runner);
  OSP_DCHECK(cast_trust_store_);
  OSP_DCHECK(crl_trust_store_);
}

SenderSocketFactory::~SenderSocketFactory() {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
}

void SenderSocketFactory::set_factory(TlsConnectionFactory* factory) {
  OSP_DCHECK(factory);
  factory_ = factory;
}

void SenderSocketFactory::Connect(const IPEndpoint& endpoint,
                                  DeviceMediaPolicy media_policy,
                                  CastSocket::Client* client) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(client);
  auto it = FindPendingConnection(endpoint);
  if (it == pending_connections_.end()) {
    pending_connections_.emplace_back(
        PendingConnection{endpoint, media_policy, client});
    factory_->Connect(endpoint, TlsConnectOptions{true});
  }
}

void SenderSocketFactory::OnAccepted(
    TlsConnectionFactory* factory,
    std::vector<uint8_t> der_x509_peer_cert,
    std::unique_ptr<TlsConnection> connection) {
  OSP_NOTREACHED();
  OSP_LOG_FATAL << "This factory is connect-only";
}

void SenderSocketFactory::OnConnected(
    TlsConnectionFactory* factory,
    std::vector<uint8_t> der_x509_peer_cert,
    std::unique_ptr<TlsConnection> connection) {
  const IPEndpoint& endpoint = connection->GetRemoteEndpoint();
  auto it = FindPendingConnection(endpoint);
  if (it == pending_connections_.end()) {
    OSP_DLOG_ERROR << "TLS connection succeeded for unknown endpoint: "
                   << endpoint;
    return;
  }
  DeviceMediaPolicy media_policy = it->media_policy;
  CastSocket::Client* client = it->client;
  pending_connections_.erase(it);

  ErrorOr<std::unique_ptr<ParsedCertificate>> peer_cert =
      ParsedCertificate::ParseFromDER(der_x509_peer_cert);
  if (!peer_cert) {
    client_->OnError(this, endpoint, peer_cert.error());
    return;
  }

  auto socket =
      MakeSerialDelete<CastSocket>(task_runner_, std::move(connection), this);
  pending_auth_.emplace_back(
      new PendingAuth{endpoint, media_policy, std::move(socket), client,
                      std::make_unique<AuthContext>(AuthContext::Create()),
                      std::move(peer_cert.value())});
  PendingAuth& pending = *pending_auth_.back();

  CastMessage auth_challenge =
      CreateAuthChallengeMessage(*pending.auth_context);
  Error error = pending.socket->Send(auth_challenge);
  if (!error.ok()) {
    pending_auth_.pop_back();
    client_->OnError(this, endpoint, error);
  }
}

void SenderSocketFactory::OnConnectionFailed(TlsConnectionFactory* factory,
                                             const IPEndpoint& remote_address) {
  auto it = FindPendingConnection(remote_address);
  if (it == pending_connections_.end()) {
    return;
  }
  pending_connections_.erase(it);
  client_->OnError(this, remote_address, Error::Code::kConnectionFailed);
}

void SenderSocketFactory::OnError(TlsConnectionFactory* factory, Error error) {
  std::vector<PendingConnection> connections;
  pending_connections_.swap(connections);
  for (const PendingConnection& pending : connections) {
    client_->OnError(this, pending.endpoint, error);
  }
}

std::vector<SenderSocketFactory::PendingConnection>::iterator
SenderSocketFactory::FindPendingConnection(const IPEndpoint& endpoint) {
  return std::find_if(pending_connections_.begin(), pending_connections_.end(),
                      [&endpoint](const PendingConnection& pending) {
                        return pending.endpoint == endpoint;
                      });
}

void SenderSocketFactory::OnError(CastSocket* socket, Error error) {
  auto it = std::find_if(pending_auth_.begin(), pending_auth_.end(),
                         [id = socket->socket_id()](
                             const std::unique_ptr<PendingAuth>& pending_auth) {
                           return pending_auth->socket->socket_id() == id;
                         });
  if (it == pending_auth_.end()) {
    OSP_DLOG_ERROR << "Got error for unknown pending socket";
    return;
  }
  IPEndpoint endpoint = (*it)->endpoint;
  pending_auth_.erase(it);
  client_->OnError(this, endpoint, error);
}

void SenderSocketFactory::OnMessage(CastSocket* socket, CastMessage message) {
  auto it = std::find_if(pending_auth_.begin(), pending_auth_.end(),
                         [id = socket->socket_id()](
                             const std::unique_ptr<PendingAuth>& pending_auth) {
                           return pending_auth->socket->socket_id() == id;
                         });
  if (it == pending_auth_.end()) {
    OSP_DLOG_ERROR << "Got message for unknown pending socket";
    return;
  }

  std::unique_ptr<PendingAuth> pending = std::move(*it);
  pending_auth_.erase(it);
  if (!IsAuthMessage(message)) {
    client_->OnError(this, pending->endpoint,
                     Error::Code::kCastV2AuthenticationError);
    return;
  }

  ErrorOr<CastDeviceCertPolicy> policy_or_error = AuthenticateChallengeReply(
      message, *pending->peer_cert, *pending->auth_context,
      cast_trust_store_.get(), crl_trust_store_.get());
  if (policy_or_error.is_error()) {
    OSP_DLOG_WARN << "Authentication failed for " << pending->endpoint
                  << " with error: " << policy_or_error.error();
    client_->OnError(this, pending->endpoint, policy_or_error.error());
    return;
  }

  if (policy_or_error.value() == CastDeviceCertPolicy::kAudioOnly &&
      pending->media_policy == DeviceMediaPolicy::kIncludesVideo) {
    client_->OnError(this, pending->endpoint,
                     Error::Code::kCastV2ChannelPolicyMismatch);
    return;
  }
  pending->socket->set_audio_only(policy_or_error.value() ==
                                  CastDeviceCertPolicy::kAudioOnly);

  pending->socket->SetClient(pending->client);
  client_->OnConnected(this, pending->endpoint,
                       std::unique_ptr<CastSocket>(pending->socket.release()));
}

}  // namespace cast
}  // namespace openscreen
