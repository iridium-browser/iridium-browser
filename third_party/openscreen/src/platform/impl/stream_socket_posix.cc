// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/stream_socket_posix.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace openscreen {

namespace {
constexpr int kDefaultMaxBacklogSize = 64;

// Call Select with no timeout, so that it doesn't block. Then use the result
// to determine if any connection is pending.
bool IsConnectionPending(int fd) {
  fd_set handle_set;
  FD_ZERO(&handle_set);
  FD_SET(fd, &handle_set);
  struct timeval tv {
    0
  };
  return select(fd + 1, &handle_set, nullptr, nullptr, &tv) > 0;
}
}  // namespace

StreamSocketPosix::StreamSocketPosix(IPAddress::Version version)
    : version_(version) {}

StreamSocketPosix::StreamSocketPosix(const IPEndpoint& local_endpoint)
    : version_(local_endpoint.address.version()),
      local_address_(local_endpoint) {}

StreamSocketPosix::StreamSocketPosix(SocketAddressPosix local_address,
                                     IPEndpoint remote_address,
                                     int file_descriptor)
    : handle_(file_descriptor),
      version_(local_address.version()),
      local_address_(local_address),
      remote_address_(remote_address),
      state_(TcpSocketState::kConnected) {
  Initialize();
}

StreamSocketPosix::StreamSocketPosix(StreamSocketPosix&& other) noexcept =
    default;
StreamSocketPosix& StreamSocketPosix::operator=(StreamSocketPosix&& other) =
    default;

StreamSocketPosix::~StreamSocketPosix() {
  if (handle_.fd != kUnsetHandleFd) {
    OSP_DCHECK(state_ != TcpSocketState::kClosed);
    Close();
  }
}

WeakPtr<StreamSocketPosix> StreamSocketPosix::GetWeakPtr() const {
  return weak_factory_.GetWeakPtr();
}

ErrorOr<std::unique_ptr<StreamSocket>> StreamSocketPosix::Accept() {
  if (!EnsureInitializedAndOpen()) {
    return ReportSocketClosedError();
  }

  if (!is_bound_ || state_ != TcpSocketState::kListening) {
    return CloseOnError(Error::Code::kSocketInvalidState);
  }

  // Check if any connection is pending, and return a special error code if not.
  if (!IsConnectionPending(handle_.fd)) {
    return Error::Code::kAgain;
  }

  // We copy our address to new_remote_address since it should be in the same
  // family. The accept call will overwrite it.
  SocketAddressPosix new_remote_address = local_address_.value();
  socklen_t remote_address_size = new_remote_address.size();
  const int new_file_descriptor =
      accept(handle_.fd, new_remote_address.address(), &remote_address_size);
  if (new_file_descriptor == kUnsetHandleFd) {
    return CloseOnError(
        Error(Error::Code::kSocketAcceptFailure, strerror(errno)));
  }
  new_remote_address.RecomputeEndpoint();

  return ErrorOr<std::unique_ptr<StreamSocket>>(
      std::make_unique<StreamSocketPosix>(local_address_.value(),
                                          new_remote_address.endpoint(),
                                          new_file_descriptor));
}

Error StreamSocketPosix::Bind() {
  if (!local_address_.has_value()) {
    return CloseOnError(Error::Code::kSocketInvalidState);
  }

  if (!EnsureInitializedAndOpen()) {
    return ReportSocketClosedError();
  }

  if (is_bound_) {
    return CloseOnError(Error::Code::kSocketInvalidState);
  }

  if (bind(handle_.fd, local_address_.value().address(),
           local_address_.value().size()) != 0) {
    return CloseOnError(
        Error(Error::Code::kSocketBindFailure, strerror(errno)));
  }

  is_bound_ = true;
  return Error::None();
}

Error StreamSocketPosix::Close() {
  if (handle_.fd == kUnsetHandleFd) {
    return ReportSocketClosedError();
  }

  OSP_DCHECK(state_ != TcpSocketState::kClosed);
  state_ = TcpSocketState::kClosed;

  const int file_descriptor_to_close = handle_.fd;
  handle_.fd = kUnsetHandleFd;
  if (close(file_descriptor_to_close) != 0) {
    return last_error_code_ = Error::Code::kSocketInvalidState;
  }

  return Error::None();
}

Error StreamSocketPosix::Connect(const IPEndpoint& remote_endpoint) {
  if (!EnsureInitializedAndOpen()) {
    return ReportSocketClosedError();
  }

  SocketAddressPosix address(remote_endpoint);
  int ret = connect(handle_.fd, address.address(), address.size());
  if (ret != 0 && errno != EINPROGRESS) {
    return CloseOnError(
        Error(Error::Code::kSocketConnectFailure, strerror(errno)));
  }

  if (!is_bound_) {
    if (local_address_.has_value()) {
      return CloseOnError(Error::Code::kSocketInvalidState);
    }

    struct sockaddr_in6 address_in6;
    socklen_t size = sizeof(address_in6);
    if (getsockname(handle_.fd,
                    reinterpret_cast<struct sockaddr*>(&address_in6),
                    &size) != 0) {
      return CloseOnError(Error::Code::kSocketConnectFailure);
    }

    local_address_.emplace(reinterpret_cast<struct sockaddr&>(address_in6));
    is_bound_ = true;
  }

  remote_address_ = remote_endpoint;
  state_ = TcpSocketState::kConnected;
  return Error::None();
}

Error StreamSocketPosix::Listen() {
  return Listen(kDefaultMaxBacklogSize);
}

Error StreamSocketPosix::Listen(int max_backlog_size) {
  OSP_DCHECK(state_ == TcpSocketState::kNotConnected);
  if (!EnsureInitializedAndOpen()) {
    return ReportSocketClosedError();
  }

  if (listen(handle_.fd, max_backlog_size) != 0) {
    return CloseOnError(
        Error(Error::Code::kSocketListenFailure, strerror(errno)));
  }

  state_ = TcpSocketState::kListening;
  return Error::None();
}

absl::optional<IPEndpoint> StreamSocketPosix::remote_address() const {
  if ((state_ != TcpSocketState::kConnected) || !remote_address_) {
    return absl::nullopt;
  }
  return remote_address_.value();
}

absl::optional<IPEndpoint> StreamSocketPosix::local_address() const {
  if (!local_address_.has_value()) {
    return absl::nullopt;
  }
  return local_address_.value().endpoint();
}

TcpSocketState StreamSocketPosix::state() const {
  return state_;
}

IPAddress::Version StreamSocketPosix::version() const {
  return version_;
}

bool StreamSocketPosix::EnsureInitializedAndOpen() {
  if (state_ == TcpSocketState::kNotConnected &&
      (handle_.fd == kUnsetHandleFd) &&
      (last_error_code_ == Error::Code::kNone)) {
    return Initialize() == Error::None();
  }

  return handle_.fd != kUnsetHandleFd;
}

Error StreamSocketPosix::Initialize() {
  if (handle_.fd == kUnsetHandleFd) {
    int domain;
    switch (version_) {
      case IPAddress::Version::kV4:
        domain = AF_INET;
        break;
      case IPAddress::Version::kV6:
        domain = AF_INET6;
        break;
    }

    handle_.fd = socket(domain, SOCK_STREAM, 0);
    if (handle_.fd == kUnsetHandleFd) {
      return last_error_code_ = Error::Code::kSocketInvalidState;
    }
  }

  const int current_flags = fcntl(handle_.fd, F_GETFL, 0);
  if (fcntl(handle_.fd, F_SETFL, current_flags | O_NONBLOCK) == -1) {
    return CloseOnError(Error::Code::kSocketInvalidState);
  }

  OSP_DCHECK_EQ(last_error_code_, Error::Code::kNone);
  return Error::None();
}

Error StreamSocketPosix::CloseOnError(Error error) {
  last_error_code_ = error.code();
  Close();
  return error;
}

// If is_open is false, the socket has either not been initialized
// or has been closed, either on purpose or due to error.
Error StreamSocketPosix::ReportSocketClosedError() {
  return last_error_code_ = Error::Code::kSocketClosedFailure;
}
}  // namespace openscreen
