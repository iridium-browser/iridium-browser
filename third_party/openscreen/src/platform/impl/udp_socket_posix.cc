// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/udp_socket_posix.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/types/optional.h"
#include "platform/api/task_runner.h"
#include "platform/base/error.h"
#include "platform/impl/udp_socket_reader_posix.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace {

// 64 KB is the maximum possible UDP datagram size.
#if !defined(OS_LINUX)
constexpr int kMaxUdpBufferSize = 64 << 10;
#endif

constexpr bool IsPowerOf2(uint32_t x) {
  return (x > 0) && ((x & (x - 1)) == 0);
}

static_assert(IsPowerOf2(alignof(struct cmsghdr)),
              "std::align requires power-of-2 alignment");

using IPv4NetworkInterfaceIndex = decltype(ip_mreqn().imr_ifindex);
using IPv6NetworkInterfaceIndex = decltype(ipv6_mreq().ipv6mr_interface);

ErrorOr<int> CreateNonBlockingUdpSocket(int domain) {
  int fd = socket(domain, SOCK_DGRAM, 0);
  if (fd == -1) {
    return Error(Error::Code::kInitializationFailure, strerror(errno));
  }
  // On non-Linux, the SOCK_NONBLOCK option is not available, so use the
  // more-portable method of calling fcntl() to set this behavior.
  if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
    close(fd);
    return Error(Error::Code::kInitializationFailure, strerror(errno));
  }
  return fd;
}

}  // namespace

UdpSocketPosix::UdpSocketPosix(TaskRunner* task_runner,
                               Client* client,
                               SocketHandle handle,
                               const IPEndpoint& local_endpoint,
                               PlatformClientPosix* platform_client)
    : task_runner_(task_runner),
      client_(client),
      handle_(handle),
      local_endpoint_(local_endpoint),
      platform_client_(platform_client) {
  OSP_DCHECK(task_runner_);
  OSP_DCHECK(local_endpoint_.address.IsV4() || local_endpoint_.address.IsV6());

  if (handle_.fd >= 0) {
    if (platform_client_) {
      platform_client_->udp_socket_reader()->OnCreate(this);
    }
  }
}

UdpSocketPosix::~UdpSocketPosix() {
  Close();
}

const SocketHandle& UdpSocketPosix::GetHandle() const {
  return handle_;
}

// static
ErrorOr<std::unique_ptr<UdpSocket>> UdpSocket::Create(
    TaskRunner* task_runner,
    Client* client,
    const IPEndpoint& endpoint) {
  static std::atomic_bool in_create{false};
  const bool in_create_local = in_create.exchange(true);
  OSP_DCHECK_EQ(in_create_local, false)
      << "Another UdpSocket::Create call is in progress. Calls to this method "
         "must be seralized.";

  if (in_create_local) {
    return Error::Code::kAgain;
  }

  int domain;
  switch (endpoint.address.version()) {
    case Version::kV4:
      domain = AF_INET;
      break;
    case Version::kV6:
      domain = AF_INET6;
      break;
  }
  const ErrorOr<int> fd = CreateNonBlockingUdpSocket(domain);
  if (!fd) {
    in_create = false;
    return fd.error();
  }

  std::unique_ptr<UdpSocket> socket = std::make_unique<UdpSocketPosix>(
      task_runner, client, SocketHandle(fd.value()), endpoint);
  in_create = false;
  return socket;
}

bool UdpSocketPosix::IsIPv4() const {
  return local_endpoint_.address.IsV4();
}

bool UdpSocketPosix::IsIPv6() const {
  return local_endpoint_.address.IsV6();
}

IPEndpoint UdpSocketPosix::GetLocalEndpoint() const {
  if (local_endpoint_.port == 0) {
    // Note: If the getsockname() call fails, just assume that's because the
    // socket isn't bound yet. In this case, leave the original value in-place.
    switch (local_endpoint_.address.version()) {
      case UdpSocket::Version::kV4: {
        struct sockaddr_in address;
        socklen_t address_len = sizeof(address);
        if (getsockname(handle_.fd,
                        reinterpret_cast<struct sockaddr*>(&address),
                        &address_len) == 0) {
          OSP_DCHECK_EQ(address.sin_family, AF_INET);
          local_endpoint_.address =
              IPAddress(IPAddress::Version::kV4,
                        reinterpret_cast<uint8_t*>(&address.sin_addr.s_addr));
          local_endpoint_.port = ntohs(address.sin_port);
        }
        break;
      }

      case UdpSocket::Version::kV6: {
        struct sockaddr_in6 address;
        socklen_t address_len = sizeof(address);
        if (getsockname(handle_.fd,
                        reinterpret_cast<struct sockaddr*>(&address),
                        &address_len) == 0) {
          OSP_DCHECK_EQ(address.sin6_family, AF_INET6);
          local_endpoint_.address =
              IPAddress(IPAddress::Version::kV6,
                        reinterpret_cast<uint8_t*>(&address.sin6_addr));
          local_endpoint_.port = ntohs(address.sin6_port);
        }
        break;
      }
    }
  }

  return local_endpoint_;
}

void UdpSocketPosix::Bind() {
  if (is_closed()) {
    OnError(Error::Code::kSocketClosedFailure);
    return;
  }

  // This is effectively a boolean passed to setsockopt() to allow a future
  // bind() on the same socket to succeed, even if the address is already in
  // use. This is pretty much universally the desired behavior.
  const int reuse_addr = 1;
  if (setsockopt(handle_.fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
                 sizeof(reuse_addr)) == -1) {
    OnError(Error::Code::kSocketOptionSettingFailure);
  }

  bool is_bound = false;
  switch (local_endpoint_.address.version()) {
    case UdpSocket::Version::kV4: {
      struct sockaddr_in address {};
      address.sin_family = AF_INET;
      address.sin_port = htons(local_endpoint_.port);
      local_endpoint_.address.CopyToV4(
          reinterpret_cast<uint8_t*>(&address.sin_addr.s_addr));
      if (bind(handle_.fd, reinterpret_cast<struct sockaddr*>(&address),
               sizeof(address)) != -1) {
        is_bound = true;
      }
    } break;

    case UdpSocket::Version::kV6: {
      struct sockaddr_in6 address {};
      address.sin6_family = AF_INET6;
      address.sin6_port = htons(local_endpoint_.port);
      local_endpoint_.address.CopyToV6(
          reinterpret_cast<uint8_t*>(&address.sin6_addr));
      if (bind(handle_.fd, reinterpret_cast<struct sockaddr*>(&address),
               sizeof(address)) != -1) {
        is_bound = true;
      }
    } break;
  }

  if (is_bound) {
    client_->OnBound(this);
  } else {
    OnError(Error::Code::kSocketBindFailure);
  }
}

void UdpSocketPosix::SetMulticastOutboundInterface(
    NetworkInterfaceIndex ifindex) {
  if (is_closed()) {
    OnError(Error::Code::kSocketClosedFailure);
    return;
  }

  switch (local_endpoint_.address.version()) {
    case UdpSocket::Version::kV4: {
      struct ip_mreqn multicast_properties;
      // Appropriate address is set based on |imr_ifindex| when set.
      multicast_properties.imr_address.s_addr = INADDR_ANY;
      multicast_properties.imr_multiaddr.s_addr = INADDR_ANY;
      multicast_properties.imr_ifindex =
          static_cast<IPv4NetworkInterfaceIndex>(ifindex);
      if (setsockopt(handle_.fd, IPPROTO_IP, IP_MULTICAST_IF,
                     &multicast_properties,
                     sizeof(multicast_properties)) == -1) {
        OnError(Error::Code::kSocketOptionSettingFailure);
      }
      return;
    }

    case UdpSocket::Version::kV6: {
      const auto index = static_cast<IPv6NetworkInterfaceIndex>(ifindex);
      if (setsockopt(handle_.fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index,
                     sizeof(index)) == -1) {
        OnError(Error::Code::kSocketOptionSettingFailure);
      }
      return;
    }
  }

  OSP_NOTREACHED();
}

void UdpSocketPosix::JoinMulticastGroup(const IPAddress& address,
                                        NetworkInterfaceIndex ifindex) {
  if (is_closed()) {
    OnError(Error::Code::kSocketClosedFailure);
    return;
  }

  switch (local_endpoint_.address.version()) {
    case UdpSocket::Version::kV4: {
      // Passed as data to setsockopt().  1 means return IP_PKTINFO control data
      // in recvmsg() calls.
      const int enable_pktinfo = 1;
      if (setsockopt(handle_.fd, IPPROTO_IP, IP_PKTINFO, &enable_pktinfo,
                     sizeof(enable_pktinfo)) == -1) {
        OnError(Error::Code::kSocketOptionSettingFailure);
        return;
      }
      struct ip_mreqn multicast_properties;
      // Appropriate address is set based on |imr_ifindex| when set.
      multicast_properties.imr_address.s_addr = INADDR_ANY;
      multicast_properties.imr_ifindex =
          static_cast<IPv4NetworkInterfaceIndex>(ifindex);
      static_assert(sizeof(multicast_properties.imr_multiaddr) == 4u,
                    "IPv4 address requires exactly 4 bytes");
      address.CopyToV4(
          reinterpret_cast<uint8_t*>(&multicast_properties.imr_multiaddr));
      if (setsockopt(handle_.fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     &multicast_properties,
                     sizeof(multicast_properties)) == -1) {
        OnError(Error::Code::kSocketOptionSettingFailure);
      }
      return;
    }

    case UdpSocket::Version::kV6: {
      // Passed as data to setsockopt().  1 means return IPV6_PKTINFO control
      // data in recvmsg() calls.
      const int enable_pktinfo = 1;
      if (setsockopt(handle_.fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
                     &enable_pktinfo, sizeof(enable_pktinfo)) == -1) {
        OnError(Error::Code::kSocketOptionSettingFailure);
        return;
      }
      struct ipv6_mreq multicast_properties = {
          {/* filled-in below */},
          static_cast<IPv6NetworkInterfaceIndex>(ifindex),
      };
      static_assert(sizeof(multicast_properties.ipv6mr_multiaddr) == 16u,
                    "IPv6 address requires exactly 16 bytes");
      address.CopyToV6(
          reinterpret_cast<uint8_t*>(&multicast_properties.ipv6mr_multiaddr));
      // Portability note: All platforms support IPV6_JOIN_GROUP, which is
      // synonymous with IPV6_ADD_MEMBERSHIP.
      if (setsockopt(handle_.fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                     &multicast_properties,
                     sizeof(multicast_properties)) == -1) {
        OnError(Error::Code::kSocketOptionSettingFailure);
      }
      return;
    }
  }

  OSP_NOTREACHED();
}

namespace {

// Examine |posix_errno| to determine whether the specific cause of a failure
// was transient or hard, and return the appropriate error response.
Error ChooseError(decltype(errno) posix_errno, Error::Code hard_error_code) {
  if (posix_errno == EAGAIN || posix_errno == EWOULDBLOCK ||
      posix_errno == ENOBUFS) {
    return Error(Error::Code::kAgain, strerror(errno));
  }
  return Error(hard_error_code, strerror(errno));
}

IPAddress GetIPAddressFromSockAddr(const sockaddr_in& sa) {
  static_assert(IPAddress::kV4Size == sizeof(sa.sin_addr.s_addr),
                "IPv4 address size mismatch.");
  return IPAddress(IPAddress::Version::kV4,
                   reinterpret_cast<const uint8_t*>(&sa.sin_addr.s_addr));
}

IPAddress GetIPAddressFromPktInfo(const in_pktinfo& pktinfo) {
  static_assert(IPAddress::kV4Size == sizeof(pktinfo.ipi_addr),
                "IPv4 address size mismatch.");
  return IPAddress(IPAddress::Version::kV4,
                   reinterpret_cast<const uint8_t*>(&pktinfo.ipi_addr));
}

uint16_t GetPortFromFromSockAddr(const sockaddr_in& sa) {
  return ntohs(sa.sin_port);
}

IPAddress GetIPAddressFromSockAddr(const sockaddr_in6& sa) {
  return IPAddress(IPAddress::Version::kV6, sa.sin6_addr.s6_addr);
}

IPAddress GetIPAddressFromPktInfo(const in6_pktinfo& pktinfo) {
  return IPAddress(IPAddress::Version::kV6, pktinfo.ipi6_addr.s6_addr);
}

uint16_t GetPortFromFromSockAddr(const sockaddr_in6& sa) {
  return ntohs(sa.sin6_port);
}

template <class PktInfoType>
bool IsPacketInfo(cmsghdr* cmh);

template <>
bool IsPacketInfo<in_pktinfo>(cmsghdr* cmh) {
  return cmh->cmsg_level == IPPROTO_IP && cmh->cmsg_type == IP_PKTINFO;
}

template <>
bool IsPacketInfo<in6_pktinfo>(cmsghdr* cmh) {
  return cmh->cmsg_level == IPPROTO_IPV6 && cmh->cmsg_type == IPV6_PKTINFO;
}

template <class SockAddrType, class PktInfoType>
ErrorOr<UdpPacket> ReceiveMessageInternal(int fd) {
  int upper_bound_bytes;
#if defined(OS_LINUX)
  // This should return the exact size of the next message.
  upper_bound_bytes = recv(fd, nullptr, 0, MSG_PEEK | MSG_TRUNC);
  if (upper_bound_bytes == -1) {
    return ChooseError(errno, Error::Code::kSocketReadFailure);
  }
#elif defined(MAC_OSX)
  // Can't use MSG_TRUNC in recv(). Use the FIONREAD ioctl() to get an
  // upper-bound.
  if (ioctl(fd, FIONREAD, &upper_bound_bytes) == -1 || upper_bound_bytes < 0) {
    return ChooseError(errno, Error::Code::kSocketReadFailure);
  }
  upper_bound_bytes = std::min(upper_bound_bytes, kMaxUdpBufferSize);
#else  // Other POSIX platforms (neither MSG_TRUNC nor FIONREAD available).
  upper_bound_bytes = kMaxUdpBufferSize;
#endif

  UdpPacket packet(upper_bound_bytes);
  msghdr msg = {};
  SockAddrType sa;
  msg.msg_name = &sa;
  msg.msg_namelen = sizeof(sa);
  iovec iov = {packet.data(), packet.size()};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  // Although we don't do anything with the control buffer, on Linux
  // it is required for the message to be properly read.
#if defined(OS_LINUX)
  alignas(alignof(cmsghdr)) uint8_t control_buffer[1024];
  msg.msg_control = control_buffer;
  msg.msg_controllen = sizeof(control_buffer);
#endif
  const ssize_t bytes_received = recvmsg(fd, &msg, 0);
  if (bytes_received == -1) {
    OSP_DVLOG << "Failed to read from socket.";
    return ChooseError(errno, Error::Code::kSocketReadFailure);
  }
  // We may not populate the entire packet.
  OSP_DCHECK_LE(static_cast<size_t>(bytes_received), packet.size());
  packet.resize(bytes_received);

  IPEndpoint source_endpoint = {.address = GetIPAddressFromSockAddr(sa),
                                .port = GetPortFromFromSockAddr(sa)};
  packet.set_source(std::move(source_endpoint));

  // For multicast sockets, the packet's original destination address may be
  // the host address (since we called bind()) but it may also be a
  // multicast address.  This may be relevant for handling multicast data;
  // specifically, mDNSResponder requires this information to work properly.

  socklen_t sa_len = sizeof(sa);
  if (((msg.msg_flags & MSG_CTRUNC) != 0) ||
      (getsockname(fd, reinterpret_cast<sockaddr*>(&sa), &sa_len) == -1)) {
    return Error::Code::kNone;
  }
  for (cmsghdr* cmh = CMSG_FIRSTHDR(&msg); cmh; cmh = CMSG_NXTHDR(&msg, cmh)) {
    if (IsPacketInfo<PktInfoType>(cmh)) {
      PktInfoType* pktinfo = reinterpret_cast<PktInfoType*>(CMSG_DATA(cmh));
      IPEndpoint destination_endpoint = {
          .address = GetIPAddressFromPktInfo(*pktinfo),
          .port = GetPortFromFromSockAddr(sa)};
      packet.set_destination(std::move(destination_endpoint));
      break;
    }
  }
  return std::move(packet);
}

}  // namespace

void UdpSocketPosix::ReceiveMessage() {
  // WARNING: This method may be called on a different thread from the thread
  // calling into all the other methods.

  if (is_closed()) {
    task_runner_->PostTask([weak_this = weak_factory_.GetWeakPtr()] {
      if (auto* self = weak_this.get()) {
        if (auto* client = self->client_) {
          client->OnRead(self, Error::Code::kSocketClosedFailure);
        }
      }
    });
    return;
  }

  ErrorOr<UdpPacket> read_result = Error::Code::kUnknownError;
  switch (local_endpoint_.address.version()) {
    case UdpSocket::Version::kV4: {
      read_result = ReceiveMessageInternal<sockaddr_in, in_pktinfo>(handle_.fd);
      break;
    }
    case UdpSocket::Version::kV6: {
      read_result =
          ReceiveMessageInternal<sockaddr_in6, in6_pktinfo>(handle_.fd);
      break;
    }
    default: {
      OSP_NOTREACHED();
    }
  }

  task_runner_->PostTask([weak_this = weak_factory_.GetWeakPtr(),
                          read_result = std::move(read_result)]() mutable {
    if (auto* self = weak_this.get()) {
      if (auto* client = self->client_) {
        client->OnRead(self, std::move(read_result));
      }
    }
  });
}

void UdpSocketPosix::SendMessage(const void* data,
                                 size_t length,
                                 const IPEndpoint& dest) {
  if (is_closed()) {
    if (client_) {
      client_->OnSendError(this, Error::Code::kSocketClosedFailure);
    }
    return;
  }

  struct iovec iov = {const_cast<void*>(data), length};
  struct msghdr msg;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  ssize_t num_bytes_sent = -2;
  switch (local_endpoint_.address.version()) {
    case UdpSocket::Version::kV4: {
      struct sockaddr_in sa {};
      sa.sin_family = AF_INET;
      sa.sin_port = htons(dest.port);
      dest.address.CopyToV4(reinterpret_cast<uint8_t*>(&sa.sin_addr.s_addr));
      msg.msg_name = &sa;
      msg.msg_namelen = sizeof(sa);
      num_bytes_sent = sendmsg(handle_.fd, &msg, 0);
      break;
    }

    case UdpSocket::Version::kV6: {
      struct sockaddr_in6 sa {};
      sa.sin6_family = AF_INET6;
      sa.sin6_port = htons(dest.port);
      dest.address.CopyToV6(reinterpret_cast<uint8_t*>(&sa.sin6_addr.s6_addr));
      msg.msg_name = &sa;
      msg.msg_namelen = sizeof(sa);
      num_bytes_sent = sendmsg(handle_.fd, &msg, 0);
      break;
    }
  }

  if (num_bytes_sent == -1) {
    if (client_) {
      client_->OnSendError(this,
                           ChooseError(errno, Error::Code::kSocketSendFailure));
    }
    return;
  }

  // Sanity-check: UDP datagram sendmsg() is all or nothing.
  OSP_DCHECK_EQ(static_cast<size_t>(num_bytes_sent), length);
}

void UdpSocketPosix::SetDscp(UdpSocket::DscpMode state) {
  if (is_closed()) {
    OnError(Error::Code::kSocketClosedFailure);
    return;
  }

  constexpr auto kSettingLevel = IPPROTO_IP;
  uint8_t code_array[1] = {static_cast<uint8_t>(state)};
  auto code = setsockopt(handle_.fd, kSettingLevel, IP_TOS, code_array,
                         sizeof(uint8_t));

  if (code == EBADF || code == ENOTSOCK || code == EFAULT) {
    OSP_VLOG << "BAD SOCKET PROVIDED. CODE: " << code;
    OnError(Error::Code::kSocketOptionSettingFailure);
  } else if (code == EINVAL) {
    OSP_VLOG << "INVALID DSCP INFO PROVIDED";
    OnError(Error::Code::kSocketOptionSettingFailure);
  } else if (code == ENOPROTOOPT) {
    OSP_VLOG << "INVALID DSCP SETTING LEVEL PROVIDED: " << kSettingLevel;
    OnError(Error::Code::kSocketOptionSettingFailure);
  }
}

void UdpSocketPosix::OnError(Error::Code error_code) {
  // The call to Close() may change |errno|, so save it here.
  const auto original_errno = errno;

  // Close the socket unless the error code represents a transient condition.
  if (error_code != Error::Code::kNone && error_code != Error::Code::kAgain) {
    Close();
  }

  if (client_) {
    // Call the thread-safe strerror_r() to get the human-readable form of
    // |errno|. This is a real mess: 1. Since there seems to be no constant
    // defined for the maximum buffer size in the standard library, 1024 is
    // used, as suggested by the man page for strerror_r(). 2. There are two
    // possible versions of this function: The POSIX one returns int(0) on
    // success, while the legacy GNU-specific one will provide a non-null char
    // pointer (that may or may not be within the |buffer|).
    char buffer[1024];
    const auto result = strerror_r(original_errno, buffer, sizeof(buffer));
    const char* errno_str;
    if (std::is_convertible<decltype(result), int>::value &&
        !result) {  // Case 1: POSIX strerror_r() success.
      errno_str = buffer;
    } else if (std::is_convertible<decltype(result), const char*>::value &&
               result) {  // Case 2: GNU strerror_r() success.
      errno_str = reinterpret_cast<const char*>(result);
    } else {  // Case 3: strerror_r() failed (either version).
      buffer[0] = '\0';
      errno_str = buffer;
    }

    std::stringstream stream;
    stream << "endpoint: " << local_endpoint_ << ", error: " << errno_str;
    client_->OnError(this, Error(error_code, stream.str()));
  }
}

void UdpSocketPosix::Close() {
  if (handle_.fd < 0) {
    return;
  }

  // Notify the UdpSocketReaderPosix that the socket handle is about to be
  // closed.
  if (platform_client_) {
    platform_client_->udp_socket_reader()->OnDestroy(this);
  }

  // It's now safe to close the socket, since no other thread (e.g., from
  // UdpSocketReaderPosix) should be inside ReceiveMessage() at this point.
  close(handle_.fd);
  handle_.fd = -1;
}

}  // namespace openscreen
