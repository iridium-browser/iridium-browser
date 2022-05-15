// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/quic_connection_factory_impl.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

#include "osp/impl/quic/quic_connection_impl.h"
#include "platform/api/task_runner.h"
#include "platform/api/time.h"
#include "platform/base/error.h"
#include "third_party/chromium_quic/src/base/location.h"
#include "third_party/chromium_quic/src/base/task_runner.h"
#include "third_party/chromium_quic/src/net/third_party/quic/core/quic_constants.h"
#include "third_party/chromium_quic/src/net/third_party/quic/platform/impl/quic_chromium_clock.h"
#include "util/osp_logging.h"
#include "util/std_util.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace osp {

class QuicTaskRunner final : public ::base::TaskRunner {
 public:
  explicit QuicTaskRunner(openscreen::TaskRunner* task_runner);
  ~QuicTaskRunner() override;

  void RunTasks();

  // base::TaskRunner overrides.
  bool PostDelayedTask(const ::base::Location& whence,
                       ::base::OnceClosure task,
                       ::base::TimeDelta delay) override;

  bool RunsTasksInCurrentSequence() const override;

 private:
  openscreen::TaskRunner* const task_runner_;
};

QuicTaskRunner::QuicTaskRunner(openscreen::TaskRunner* task_runner)
    : task_runner_(task_runner) {}

QuicTaskRunner::~QuicTaskRunner() = default;

void QuicTaskRunner::RunTasks() {}

bool QuicTaskRunner::PostDelayedTask(const ::base::Location& whence,
                                     ::base::OnceClosure task,
                                     ::base::TimeDelta delay) {
  Clock::duration wait = Clock::duration(delay.InMilliseconds());
  task_runner_->PostTaskWithDelay(
      [closure = std::move(task)]() mutable { std::move(closure).Run(); },
      wait);
  return true;
}

bool QuicTaskRunner::RunsTasksInCurrentSequence() const {
  return true;
}

QuicConnectionFactoryImpl::QuicConnectionFactoryImpl(TaskRunner* task_runner)
    : task_runner_(task_runner) {
  quic_task_runner_ = ::base::MakeRefCounted<QuicTaskRunner>(task_runner);
  alarm_factory_ = std::make_unique<::net::QuicChromiumAlarmFactory>(
      quic_task_runner_.get(), ::quic::QuicChromiumClock::GetInstance());
  ::quic::QuartcFactoryConfig factory_config;
  factory_config.alarm_factory = alarm_factory_.get();
  factory_config.clock = ::quic::QuicChromiumClock::GetInstance();
  quartc_factory_ = std::make_unique<::quic::QuartcFactory>(factory_config);
}

QuicConnectionFactoryImpl::~QuicConnectionFactoryImpl() {
  OSP_DCHECK(connections_.empty());
}

void QuicConnectionFactoryImpl::SetServerDelegate(
    ServerDelegate* delegate,
    const std::vector<IPEndpoint>& endpoints) {
  OSP_DCHECK(!delegate != !server_delegate_);

  server_delegate_ = delegate;
  sockets_.reserve(sockets_.size() + endpoints.size());

  for (const auto& endpoint : endpoints) {
    // TODO(mfoltz): Need to notify the caller and/or ServerDelegate if socket
    // create/bind errors occur. Maybe return an Error immediately, and undo
    // partial progress (i.e. "unwatch" all the sockets and call
    // sockets_.clear() to close the sockets)?
    auto create_result = UdpSocket::Create(task_runner_, this, endpoint);
    if (!create_result) {
      OSP_LOG_ERROR << "failed to create socket (for " << endpoint
                    << "): " << create_result.error().message();
      continue;
    }
    std::unique_ptr<UdpSocket> server_socket = std::move(create_result.value());
    server_socket->Bind();
    sockets_.emplace_back(std::move(server_socket));
  }
}

void QuicConnectionFactoryImpl::OnRead(UdpSocket* socket,
                                       ErrorOr<UdpPacket> packet_or_error) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicConnectionFactoryImpl::OnRead");
  if (packet_or_error.is_error()) {
    return;
  }

  UdpPacket packet = std::move(packet_or_error.value());
  // Ensure that |packet.socket| is one of the instances owned by
  // QuicConnectionFactoryImpl.
  OSP_DCHECK(ContainsIf(sockets_, [packet = std::cref(packet)](
                                      const std::unique_ptr<UdpSocket>& s) {
    return s.get() == packet.get().socket();
  }));

  // TODO(btolsch): We will need to rethink this both for ICE and connection
  // migration support.
  auto conn_it = connections_.find(packet.source());
  if (conn_it == connections_.end()) {
    if (server_delegate_) {
      OSP_VLOG << __func__ << ": spawning connection from " << packet.source();
      auto transport =
          std::make_unique<UdpTransport>(packet.socket(), packet.source());
      ::quic::QuartcSessionConfig session_config;
      session_config.perspective = ::quic::Perspective::IS_SERVER;
      session_config.packet_transport = transport.get();

      auto result = std::make_unique<QuicConnectionImpl>(
          this, server_delegate_->NextConnectionDelegate(packet.source()),
          std::move(transport),
          quartc_factory_->CreateQuartcSession(session_config));
      auto* result_ptr = result.get();
      connections_.emplace(packet.source(),
                           OpenConnection{result_ptr, packet.socket()});
      server_delegate_->OnIncomingConnection(std::move(result));
      result_ptr->OnRead(socket, std::move(packet));
    }
  } else {
    OSP_VLOG << __func__ << ": data for existing connection from "
             << packet.source();
    conn_it->second.connection->OnRead(socket, std::move(packet));
  }
}

std::unique_ptr<QuicConnection> QuicConnectionFactoryImpl::Connect(
    const IPEndpoint& endpoint,
    QuicConnection::Delegate* connection_delegate) {
  auto create_result = UdpSocket::Create(task_runner_, this, endpoint);
  if (!create_result) {
    OSP_LOG_ERROR << "failed to create socket: "
                  << create_result.error().message();
    // TODO(mfoltz): This method should return ErrorOr<uni_ptr<QuicConnection>>.
    return nullptr;
  }
  std::unique_ptr<UdpSocket> socket = std::move(create_result.value());
  auto transport = std::make_unique<UdpTransport>(socket.get(), endpoint);

  ::quic::QuartcSessionConfig session_config;
  session_config.perspective = ::quic::Perspective::IS_CLIENT;
  // TODO(btolsch): Proper server id.  Does this go in the QUIC server name
  // parameter?
  session_config.unique_remote_server_id = "turtle";
  session_config.packet_transport = transport.get();

  auto result = std::make_unique<QuicConnectionImpl>(
      this, connection_delegate, std::move(transport),
      quartc_factory_->CreateQuartcSession(session_config));

  // TODO(btolsch): This presents a problem for multihomed receivers, which may
  // register as a different endpoint in their response.  I think QUIC is
  // already tolerant of this via connection IDs but this hasn't been tested
  // (and even so, those aren't necessarily stable either).
  connections_.emplace(endpoint, OpenConnection{result.get(), socket.get()});
  sockets_.emplace_back(std::move(socket));

  return result;
}

void QuicConnectionFactoryImpl::OnConnectionClosed(QuicConnection* connection) {
  auto entry = std::find_if(
      connections_.begin(), connections_.end(),
      [connection](const decltype(connections_)::value_type& entry) {
        return entry.second.connection == connection;
      });
  OSP_DCHECK(entry != connections_.end());
  UdpSocket* const socket = entry->second.socket;
  connections_.erase(entry);

  // If none of the remaining |connections_| reference the socket, close/destroy
  // it.
  if (!ContainsIf(connections_,
                  [socket](const decltype(connections_)::value_type& entry) {
                    return entry.second.socket == socket;
                  })) {
    auto socket_it =
        std::find_if(sockets_.begin(), sockets_.end(),
                     [socket](const std::unique_ptr<UdpSocket>& s) {
                       return s.get() == socket;
                     });
    OSP_DCHECK(socket_it != sockets_.end());
    sockets_.erase(socket_it);
  }
}

void QuicConnectionFactoryImpl::OnError(UdpSocket* socket, Error error) {
  OSP_LOG_ERROR << "failed to configure socket " << error.message();
}

void QuicConnectionFactoryImpl::OnSendError(UdpSocket* socket, Error error) {
  // TODO(crbug.com/openscreen/67): Implement this method.
  OSP_UNIMPLEMENTED();
}

}  // namespace osp
}  // namespace openscreen
