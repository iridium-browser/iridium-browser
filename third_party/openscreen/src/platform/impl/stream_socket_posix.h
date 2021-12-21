// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_STREAM_SOCKET_POSIX_H_
#define PLATFORM_IMPL_STREAM_SOCKET_POSIX_H_

#include <atomic>
#include <memory>
#include <string>

#include "absl/types/optional.h"
#include "platform/base/error.h"
#include "platform/base/ip_address.h"
#include "platform/impl/socket_address_posix.h"
#include "platform/impl/socket_handle_posix.h"
#include "platform/impl/stream_socket.h"
#include "util/weak_ptr.h"

namespace openscreen {

class StreamSocketPosix : public StreamSocket {
 public:
  explicit StreamSocketPosix(IPAddress::Version version);
  explicit StreamSocketPosix(const IPEndpoint& local_endpoint);
  StreamSocketPosix(SocketAddressPosix local_address,
                    IPEndpoint remote_address,
                    int file_descriptor);

  // StreamSocketPosix is non-copyable, due to directly managing the file
  // descriptor.
  StreamSocketPosix(const StreamSocketPosix& other) = delete;
  StreamSocketPosix(StreamSocketPosix&& other) noexcept;
  StreamSocketPosix& operator=(const StreamSocketPosix& other) = delete;
  StreamSocketPosix& operator=(StreamSocketPosix&& other);
  virtual ~StreamSocketPosix();

  WeakPtr<StreamSocketPosix> GetWeakPtr() const;

  // StreamSocket overrides.
  ErrorOr<std::unique_ptr<StreamSocket>> Accept() override;
  Error Bind() override;
  Error Close() override;
  Error Connect(const IPEndpoint& remote_endpoint) override;
  Error Listen() override;
  Error Listen(int max_backlog_size) override;

  // StreamSocket getter overrides.
  const SocketHandle& socket_handle() const override { return handle_; }
  absl::optional<IPEndpoint> remote_address() const override;
  absl::optional<IPEndpoint> local_address() const override;
  TcpSocketState state() const override;
  IPAddress::Version version() const override;

 private:
  // StreamSocketPosix is lazy initialized on first usage. For simplicitly,
  // the ensure method returns a boolean of whether or not the socket was
  // initialized successfully.
  bool EnsureInitializedAndOpen();
  Error Initialize();

  Error CloseOnError(Error error);
  Error ReportSocketClosedError();

  constexpr static int kUnsetHandleFd = -1;

  // This SocketHandle object is expected to persist for the lieftime of this
  // object. The internal fd may change, but the object may not be destroyed.
  SocketHandle handle_{kUnsetHandleFd};

  // last_error_code_ is an Error::Code instead of an Error so it meets
  // atomic's (trivially) copyable and moveable requirements.
  Error::Code last_error_code_ = Error::Code::kNone;
  IPAddress::Version version_;
  absl::optional<SocketAddressPosix> local_address_;
  absl::optional<IPEndpoint> remote_address_;

  bool is_bound_ = false;
  TcpSocketState state_ = TcpSocketState::kNotConnected;

  WeakPtrFactory<StreamSocketPosix> weak_factory_{this};
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_STREAM_SOCKET_POSIX_H_
