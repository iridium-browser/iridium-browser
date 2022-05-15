// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <openssl/evp.h>
#include <openssl/mem.h>

#include <atomic>
#include <chrono>

#include "cast/common/certificate/testing/test_helpers.h"
#include "cast/common/channel/connection_namespace_handler.h"
#include "cast/common/channel/message_util.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/common/public/trust_store.h"
#include "cast/receiver/channel/device_auth_namespace_handler.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/receiver/public/receiver_socket_factory.h"
#include "cast/sender/public/sender_socket_factory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/api/tls_connection_factory.h"
#include "platform/base/tls_connect_options.h"
#include "platform/base/tls_credentials.h"
#include "platform/base/tls_listen_options.h"
#include "platform/impl/logging.h"
#include "platform/impl/network_interface.h"
#include "platform/impl/platform_client_posix.h"
#include "testing/util/task_util.h"
#include "util/crypto/certificate_utils.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

using ::testing::_;
using ::testing::StrictMock;

constexpr char kLogDecorator[] = "--- ";

}  // namespace

class SenderSocketsClient : public SenderSocketFactory::Client,
                            public VirtualConnectionRouter::SocketErrorHandler {
 public:
  explicit SenderSocketsClient(VirtualConnectionRouter* router)  // NOLINT
      : router_(router) {}
  virtual ~SenderSocketsClient() = default;

  CastSocket* socket() const { return socket_; }

  // SenderSocketFactory::Client overrides.
  void OnConnected(SenderSocketFactory* factory,
                   const IPEndpoint& endpoint,
                   std::unique_ptr<CastSocket> socket) {
    OSP_CHECK(!socket_);
    OSP_LOG_INFO << kLogDecorator
                 << "Sender connected to endpoint: " << endpoint;
    socket_ = socket.get();
    router_->TakeSocket(this, std::move(socket));
  }

  void OnError(SenderSocketFactory* factory,
               const IPEndpoint& endpoint,
               Error error) override {
    OSP_LOG_FATAL << error;
  }

  // VirtualConnectionRouter::SocketErrorHandler overrides.
  void OnClose(CastSocket* socket) override {
    socket_ = nullptr;
    OnCloseMock(socket);
  }
  void OnError(CastSocket* socket, Error error) override {
    socket_ = nullptr;
    OnErrorMock(socket, std::move(error));
  }

  MOCK_METHOD(void, OnCloseMock, (CastSocket * socket), ());
  MOCK_METHOD(void, OnErrorMock, (CastSocket * socket, Error error), ());

 private:
  VirtualConnectionRouter* const router_;
  std::atomic<CastSocket*> socket_{nullptr};
};

class ReceiverSocketsClient
    : public ReceiverSocketFactory::Client,
      public VirtualConnectionRouter::SocketErrorHandler {
 public:
  explicit ReceiverSocketsClient(VirtualConnectionRouter* router)
      : router_(router) {}
  virtual ~ReceiverSocketsClient() = default;

  const IPEndpoint& endpoint() const { return endpoint_; }
  CastSocket* socket() const { return socket_; }

  // ReceiverSocketFactory::Client overrides.
  void OnConnected(ReceiverSocketFactory* factory,
                   const IPEndpoint& endpoint,
                   std::unique_ptr<CastSocket> socket) override {
    OSP_CHECK(!socket_);
    OSP_LOG_INFO << kLogDecorator
                 << "Receiver got connection from endpoint: " << endpoint;
    endpoint_ = endpoint;
    socket_ = socket.get();
    router_->TakeSocket(this, std::move(socket));
  }

  void OnError(ReceiverSocketFactory* factory, Error error) override {
    OSP_LOG_FATAL << error;
  }

  // VirtualConnectionRouter::SocketErrorHandler overrides.
  void OnClose(CastSocket* socket) override {
    socket_ = nullptr;
    OnCloseMock(socket);
  }
  void OnError(CastSocket* socket, Error error) override {
    socket_ = nullptr;
    OnErrorMock(socket, std::move(error));
  }

  MOCK_METHOD(void, OnCloseMock, (CastSocket * socket), ());
  MOCK_METHOD(void, OnErrorMock, (CastSocket * socket, Error error), ());

 private:
  VirtualConnectionRouter* router_;
  IPEndpoint endpoint_;
  std::atomic<CastSocket*> socket_{nullptr};
};

class CastSocketE2ETest : public ::testing::Test {
 public:
  void SetUp() override {
    PlatformClientPosix::Create(std::chrono::milliseconds(10));
    task_runner_ = PlatformClientPosix::GetInstance()->GetTaskRunner();

    credentials_ =
        std::move(GenerateCredentialsForTesting("Device ID").value());

    sender_router_ = MakeSerialDelete<VirtualConnectionRouter>(task_runner_);
    sender_client_ =
        std::make_unique<StrictMock<SenderSocketsClient>>(sender_router_.get());
    sender_factory_ = MakeSerialDelete<SenderSocketFactory>(
        task_runner_, sender_client_.get(), task_runner_,
        TrustStore::CreateInstanceForTest(credentials_.root_cert_der),
        CastCRLTrustStore::Create());
    sender_tls_factory_ = SerialDeletePtr<TlsConnectionFactory>(
        task_runner_,
        TlsConnectionFactory::CreateFactory(sender_factory_.get(), task_runner_)
            .release());
    sender_factory_->set_factory(sender_tls_factory_.get());

    auth_handler_ = MakeSerialDelete<DeviceAuthNamespaceHandler>(
        task_runner_, credentials_.provider.get());
    receiver_router_ = MakeSerialDelete<VirtualConnectionRouter>(task_runner_);
    receiver_router_->AddHandlerForLocalId(kPlatformReceiverId,
                                           auth_handler_.get());
    receiver_client_ = std::make_unique<StrictMock<ReceiverSocketsClient>>(
        receiver_router_.get());
    receiver_factory_ = MakeSerialDelete<ReceiverSocketFactory>(
        task_runner_, receiver_client_.get(), receiver_router_.get());

    receiver_tls_factory_ = SerialDeletePtr<TlsConnectionFactory>(
        task_runner_, TlsConnectionFactory::CreateFactory(
                          receiver_factory_.get(), task_runner_)
                          .release());
  }

  void TearDown() override {
    OSP_LOG_INFO << "Shutting down";
    sender_router_.reset();
    receiver_router_.reset();
    receiver_tls_factory_.reset();
    receiver_factory_.reset();
    auth_handler_.reset();
    sender_tls_factory_.reset();
    sender_factory_.reset();
    PlatformClientPosix::ShutDown();
  }

 protected:
  IPAddress GetLoopbackV4Address() {
    absl::optional<InterfaceInfo> loopback = GetLoopbackInterfaceForTesting();
    OSP_CHECK(loopback);
    IPAddress address = loopback->GetIpAddressV4();
    OSP_CHECK(address);
    return address;
  }

  IPAddress GetLoopbackV6Address() {
    absl::optional<InterfaceInfo> loopback = GetLoopbackInterfaceForTesting();
    OSP_CHECK(loopback);
    IPAddress address = loopback->GetIpAddressV6();
    return address;
  }

  void Connect(const IPAddress& address) {
    uint16_t port = 65321;
    OSP_LOG_INFO << kLogDecorator << "Starting socket factories";
    task_runner_->PostTask([this, &address, port]() {
      OSP_LOG_INFO << kLogDecorator << "Receiver TLS factory Listen()";
      receiver_tls_factory_->SetListenCredentials(credentials_.tls_credentials);
      receiver_tls_factory_->Listen(IPEndpoint{address, port},
                                    TlsListenOptions{1u});
    });

    task_runner_->PostTask([this, &address, port]() {
      OSP_LOG_INFO << kLogDecorator << "Sender CastSocket factory Connect()";
      sender_factory_->Connect(IPEndpoint{address, port},
                               SenderSocketFactory::DeviceMediaPolicy::kNone,
                               sender_router_.get());
    });

    WaitForCondition([this]() { return sender_client_->socket(); });
  }

  void ConnectSocketsV4() {
    OSP_LOG_INFO << "Getting loopback IPv4 address";
    IPAddress loopback_address = GetLoopbackV4Address();
    OSP_LOG_INFO << "Connecting CastSockets";
    Connect(loopback_address);
  }

  template <typename SocketClient, typename PeerSocketClient>
  void CloseSocketsFromOneEnd(VirtualConnectionRouter* router,
                              SocketClient* client,
                              PeerSocketClient* peer_client) {
    // TODO(issuetracker.google.com/169967989): Would like to have a symmetric
    // OnClose check.
    EXPECT_CALL(*client, OnCloseMock(client->socket()));
    EXPECT_CALL(*peer_client, OnErrorMock(peer_client->socket(), _))
        .WillOnce([](CastSocket* socket, Error error) {
          EXPECT_EQ(error.code(), Error::Code::kSocketClosedFailure);
        });
    int32_t id = client->socket()->socket_id();
    std::atomic_bool did_run{false};
    task_runner_->PostTask([id, router, &did_run]() {
      router->CloseSocket(id);
      did_run = true;
    });
    OSP_LOG_INFO << "Waiting for socket to close";
    WaitForCondition([&did_run]() { return did_run.load(); });
    EXPECT_FALSE(sender_client_->socket());
    EXPECT_FALSE(receiver_client_->socket());
  }

  TaskRunner* task_runner_;

  // NOTE: Sender components.
  SerialDeletePtr<VirtualConnectionRouter> sender_router_;
  std::unique_ptr<StrictMock<SenderSocketsClient>> sender_client_;
  SerialDeletePtr<SenderSocketFactory> sender_factory_;
  SerialDeletePtr<TlsConnectionFactory> sender_tls_factory_;

  // NOTE: Receiver components.
  SerialDeletePtr<VirtualConnectionRouter> receiver_router_;
  GeneratedCredentials credentials_;
  SerialDeletePtr<DeviceAuthNamespaceHandler> auth_handler_;
  std::unique_ptr<StrictMock<ReceiverSocketsClient>> receiver_client_;
  SerialDeletePtr<ReceiverSocketFactory> receiver_factory_;
  SerialDeletePtr<TlsConnectionFactory> receiver_tls_factory_;
};

// These test the most basic setup of a complete CastSocket.  This means
// constructing both a SenderSocketFactory and ReceiverSocketFactory, making a
// TLS connection to a known port over the loopback device, and checking device
// authentication.
TEST_F(CastSocketE2ETest, ConnectV4) {
  ConnectSocketsV4();
}

TEST_F(CastSocketE2ETest, ConnectV6) {
  OSP_LOG_INFO << "Getting loopback IPv6 address";
  IPAddress loopback_address = GetLoopbackV6Address();
  if (loopback_address) {
    OSP_LOG_INFO << "Connecting CastSockets";
    Connect(loopback_address);
  } else {
    OSP_LOG_WARN << "Test skipped due to missing IPv6 loopback address";
  }
}

TEST_F(CastSocketE2ETest, SenderClose) {
  ConnectSocketsV4();

  CloseSocketsFromOneEnd(sender_router_.get(), sender_client_.get(),
                         receiver_client_.get());
}

TEST_F(CastSocketE2ETest, ReceiverClose) {
  ConnectSocketsV4();

  CloseSocketsFromOneEnd(receiver_router_.get(), receiver_client_.get(),
                         sender_client_.get());
}

}  // namespace cast
}  // namespace openscreen
