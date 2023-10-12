// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/tls_connection_factory_posix.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <openssl/ssl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <utility>
#include <vector>

#include "platform/api/task_runner.h"
#include "platform/api/tls_connection_factory.h"
#include "platform/base/tls_connect_options.h"
#include "platform/base/tls_credentials.h"
#include "platform/base/tls_listen_options.h"
#include "platform/impl/stream_socket.h"
#include "platform/impl/tls_connection_posix.h"
#include "util/crypto/certificate_utils.h"
#include "util/crypto/openssl_util.h"
#include "util/osp_logging.h"
#include "util/trace_logging.h"

namespace openscreen {

namespace {

ErrorOr<std::vector<uint8_t>> GetDEREncodedPeerCertificate(const SSL& ssl) {
  X509* const peer_cert = SSL_get_peer_certificate(&ssl);
  ErrorOr<std::vector<uint8_t>> der_peer_cert =
      ExportX509CertificateToDer(*peer_cert);
  X509_free(peer_cert);
  return der_peer_cert;
}

}  // namespace

std::unique_ptr<TlsConnectionFactory> TlsConnectionFactory::CreateFactory(
    Client* client,
    TaskRunner& task_runner) {
  return std::unique_ptr<TlsConnectionFactory>(
      new TlsConnectionFactoryPosix(client, task_runner));
}

TlsConnectionFactoryPosix::TlsConnectionFactoryPosix(
    Client* client,
    TaskRunner& task_runner,
    PlatformClientPosix* platform_client)
    : client_(client),
      task_runner_(task_runner),
      platform_client_(platform_client) {
  OSP_DCHECK(client_);
}

TlsConnectionFactoryPosix::~TlsConnectionFactoryPosix() {
  OSP_DCHECK(task_runner_.IsRunningOnTaskRunner());
  if (platform_client_) {
    platform_client_->tls_data_router()->DeregisterAcceptObserver(this);
  }
}

// TODO(issuetracker.google.com/281741213): Add support for resuming sessions.
// TODO(issuetracker.google.com/281741213): Integrate with Auth.
void TlsConnectionFactoryPosix::Connect(const IPEndpoint& remote_address,
                                        const TlsConnectOptions& options) {
  OSP_DCHECK(task_runner_.IsRunningOnTaskRunner());
  TRACE_SCOPED1(TraceCategory::kSsl, "TlsConnectionFactoryPosix::Connect",
                "remote_address", remote_address.ToString());
  IPAddress::Version version = remote_address.address.version();
  std::unique_ptr<TlsConnectionPosix> connection(
      new TlsConnectionPosix(version, task_runner_));
  Error connect_error = connection->socket_->Connect(remote_address);
  if (!connect_error.ok()) {
    TRACE_SET_RESULT(connect_error);
    DispatchConnectionFailed(remote_address);
    return;
  }

  if (!ConfigureSsl(connection.get())) {
    return;
  }

  if (options.unsafely_skip_certificate_validation) {
    // Verifies the server certificate but does not make errors fatal.
    SSL_set_verify(connection->ssl_.get(), SSL_VERIFY_NONE, nullptr);
  } else {
    // Make server certificate errors fatal.
    SSL_set_verify(connection->ssl_.get(), SSL_VERIFY_PEER, nullptr);
  }

  Connect(std::move(connection));
}

void TlsConnectionFactoryPosix::SetListenCredentials(
    const TlsCredentials& credentials) {
  OSP_DCHECK(task_runner_.IsRunningOnTaskRunner());
  EnsureInitialized();

  ErrorOr<bssl::UniquePtr<X509>> cert = ImportCertificate(
      credentials.der_x509_cert.data(), credentials.der_x509_cert.size());
  ErrorOr<bssl::UniquePtr<EVP_PKEY>> pkey =
      ImportRSAPrivateKey(credentials.der_rsa_private_key.data(),
                          credentials.der_rsa_private_key.size());

  if (!cert || !pkey ||
      SSL_CTX_use_certificate(ssl_context_.get(), cert.value().get()) != 1 ||
      SSL_CTX_use_PrivateKey(ssl_context_.get(), pkey.value().get()) != 1) {
    DispatchError(Error::Code::kSocketListenFailure);
    TRACE_SET_RESULT(Error::Code::kSocketListenFailure);
    return;
  }

  listen_credentials_set_ = true;
}

void TlsConnectionFactoryPosix::Listen(const IPEndpoint& local_address,
                                       const TlsListenOptions& options) {
  OSP_DCHECK(task_runner_.IsRunningOnTaskRunner());
  // Credentials must be set before Listen() is called.
  OSP_DCHECK(listen_credentials_set_);

  auto socket = std::make_unique<StreamSocketPosix>(local_address);
  socket->Bind();
  socket->Listen(options.backlog_size);
  if (socket->state() == TcpSocketState::kClosed) {
    DispatchError(Error::Code::kSocketListenFailure);
    TRACE_SET_RESULT(Error::Code::kSocketListenFailure);
    return;
  }
  OSP_DCHECK(socket->state() == TcpSocketState::kListening);

  OSP_DCHECK(platform_client_);
  if (platform_client_) {
    platform_client_->tls_data_router()->RegisterAcceptObserver(
        std::move(socket), this);
  }
}

void TlsConnectionFactoryPosix::OnConnectionPending(StreamSocketPosix* socket) {
  task_runner_.PostTask([connection_factory_weak_ptr =
                             weak_factory_.GetWeakPtr(),
                         socket_weak_ptr = socket->GetWeakPtr()] {
    if (!connection_factory_weak_ptr || !socket_weak_ptr) {
      // Cancel the Accept() since either the factory or the listener socket
      // went away before this task has run.
      return;
    }

    ErrorOr<std::unique_ptr<StreamSocket>> accepted = socket_weak_ptr->Accept();
    if (accepted.is_error()) {
      // Check for special error code. Because this call doesn't get executed
      // until it gets through the task runner, OnConnectionPending may get
      // called multiple times. This check ensures only the first such call will
      // create a new SSL connection.
      if (accepted.error().code() != Error::Code::kAgain) {
        connection_factory_weak_ptr->DispatchError(std::move(accepted.error()));
      }
      return;
    }

    connection_factory_weak_ptr->OnSocketAccepted(std::move(accepted.value()));
  });
}

void TlsConnectionFactoryPosix::OnSocketAccepted(
    std::unique_ptr<StreamSocket> socket) {
  OSP_DCHECK(task_runner_.IsRunningOnTaskRunner());

  TRACE_SCOPED(TraceCategory::kSsl,
               "TlsConnectionFactoryPosix::OnSocketAccepted");
  std::unique_ptr<TlsConnectionPosix> connection(
      new TlsConnectionPosix(std::move(socket), task_runner_));

  if (!ConfigureSsl(connection.get())) {
    return;
  }

  Accept(std::move(connection));
}

bool TlsConnectionFactoryPosix::ConfigureSsl(TlsConnectionPosix* connection) {
  ErrorOr<bssl::UniquePtr<SSL>> connection_result = GetSslConnection();
  if (connection_result.is_error()) {
    DispatchError(connection_result.error());
    TRACE_SET_RESULT(connection_result.error());
    return false;
  }

  bssl::UniquePtr<SSL> ssl = std::move(connection_result.value());
  if (!SSL_set_fd(ssl.get(), connection->socket_->socket_handle().fd)) {
    DispatchConnectionFailed(connection->GetRemoteEndpoint());
    TRACE_SET_RESULT(Error(Error::Code::kSocketBindFailure));
    return false;
  }

  connection->ssl_.swap(ssl);
  return true;
}

ErrorOr<bssl::UniquePtr<SSL>> TlsConnectionFactoryPosix::GetSslConnection() {
  EnsureInitialized();
  if (!ssl_context_.get()) {
    return Error::Code::kFatalSSLError;
  }

  SSL* ssl = SSL_new(ssl_context_.get());
  if (ssl == nullptr) {
    return Error::Code::kFatalSSLError;
  }

  return bssl::UniquePtr<SSL>(ssl);
}

void TlsConnectionFactoryPosix::EnsureInitialized() {
  std::call_once(init_instance_flag_, [this]() { this->Initialize(); });
}

void TlsConnectionFactoryPosix::Initialize() {
  EnsureOpenSSLInit();
  SSL_CTX* context = SSL_CTX_new(TLS_method());
  if (context == nullptr) {
    return;
  }

  SSL_CTX_set_mode(context, SSL_MODE_ENABLE_PARTIAL_WRITE);

  ssl_context_.reset(context);
}

void TlsConnectionFactoryPosix::Connect(
    std::unique_ptr<TlsConnectionPosix> connection) {
  if (connection->socket_->state() == TcpSocketState::kClosed) {
    return;
  }
  OSP_DCHECK(connection->socket_->state() == TcpSocketState::kConnected);
  ClearOpenSSLERRStack(CURRENT_LOCATION);
  const int connection_status = SSL_connect(connection->ssl_.get());
  if (connection_status != 1) {
    Error error = GetSSLError(connection->ssl_.get(), connection_status);
    if (error.code() == Error::Code::kAgain) {
      task_runner_.PostTask([weak_this = weak_factory_.GetWeakPtr(),
                             conn = std::move(connection)]() mutable {
        if (auto* self = weak_this.get()) {
          self->Connect(std::move(conn));
        }
      });
      return;
    } else {
      OSP_DVLOG << "SSL_connect failed with error: " << error;
      DispatchConnectionFailed(connection->GetRemoteEndpoint());
      TRACE_SET_RESULT(error);
      return;
    }
  }

  ErrorOr<std::vector<uint8_t>> der_peer_cert =
      GetDEREncodedPeerCertificate(*connection->ssl_);
  if (!der_peer_cert) {
    DispatchConnectionFailed(connection->GetRemoteEndpoint());
    TRACE_SET_RESULT(der_peer_cert.error());
    return;
  }

  connection->RegisterConnectionWithDataRouter(platform_client_);
  task_runner_.PostTask([weak_this = weak_factory_.GetWeakPtr(),
                         der = std::move(der_peer_cert.value()),
                         moved_connection = std::move(connection)]() mutable {
    if (auto* self = weak_this.get()) {
      self->client_->OnConnected(self, std::move(der),
                                 std::move(moved_connection));
    }
  });
}

void TlsConnectionFactoryPosix::Accept(
    std::unique_ptr<TlsConnectionPosix> connection) {
  if (connection->socket_->state() == TcpSocketState::kClosed) {
    return;
  }
  OSP_DCHECK(connection->socket_->state() == TcpSocketState::kConnected);

  ClearOpenSSLERRStack(CURRENT_LOCATION);
  const int connection_status = SSL_accept(connection->ssl_.get());
  if (connection_status != 1) {
    Error error = GetSSLError(connection->ssl_.get(), connection_status);
    if (error.code() == Error::Code::kAgain) {
      task_runner_.PostTask([weak_this = weak_factory_.GetWeakPtr(),
                             conn = std::move(connection)]() mutable {
        if (auto* self = weak_this.get()) {
          self->Accept(std::move(conn));
        }
      });
      return;
    } else {
      OSP_DVLOG << "SSL_accept failed with error: " << error;
      DispatchConnectionFailed(connection->GetRemoteEndpoint());
      TRACE_SET_RESULT(error);
      return;
    }
  }

  ErrorOr<std::vector<uint8_t>> der_peer_cert =
      GetDEREncodedPeerCertificate(*connection->ssl_);
  std::vector<uint8_t> der_peer_cert_value;
  if (der_peer_cert) {
    der_peer_cert_value = std::move(der_peer_cert.value());
  }
  connection->RegisterConnectionWithDataRouter(platform_client_);
  task_runner_.PostTask([weak_this = weak_factory_.GetWeakPtr(),
                         der = std::move(der_peer_cert_value),
                         moved_connection = std::move(connection)]() mutable {
    if (auto* self = weak_this.get()) {
      self->client_->OnAccepted(self, std::move(der),
                                std::move(moved_connection));
    }
  });
}

void TlsConnectionFactoryPosix::DispatchConnectionFailed(
    const IPEndpoint& remote_endpoint) {
  task_runner_.PostTask(
      [weak_this = weak_factory_.GetWeakPtr(), remote = remote_endpoint] {
        if (auto* self = weak_this.get()) {
          self->client_->OnConnectionFailed(self, remote);
        }
      });
}

void TlsConnectionFactoryPosix::DispatchError(Error error) {
  task_runner_.PostTask([weak_this = weak_factory_.GetWeakPtr(),
                         moved_error = std::move(error)]() mutable {
    if (auto* self = weak_this.get()) {
      self->client_->OnError(self, std::move(moved_error));
    }
  });
}

}  // namespace openscreen
