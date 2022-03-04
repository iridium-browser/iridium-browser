// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_UDP_SOCKET_H_
#define PLATFORM_API_UDP_SOCKET_H_

#include <stddef.h>  // size_t
#include <stdint.h>  // uint8_t

#include <memory>

#include "platform/api/network_interface.h"
#include "platform/base/error.h"
#include "platform/base/ip_address.h"
#include "platform/base/udp_packet.h"

namespace openscreen {

class TaskRunner;

// An open UDP socket for sending/receiving datagrams to/from either specific
// endpoints or over IP multicast.
//
// Usage: The socket is created and opened by calling the Create() method. This
// returns a unique pointer that auto-closes/destroys the socket when it goes
// out-of-scope.
class UdpSocket {
 public:
  // Client for the UdpSocket class.
  class Client {
   public:

    // Method called when the UDP socket is bound. Default implementation
    // does nothing, as clients may not care about the socket bind state.
    virtual void OnBound(UdpSocket* socket) {}

    // Method called on socket configuration operations when an error occurs.
    // These specific APIs are:
    //   UdpSocket::Bind()
    //   UdpSocket::SetMulticastOutboundInterface(...)
    //   UdpSocket::JoinMulticastGroup(...)
    //   UdpSocket::SetDscp(...)
    virtual void OnError(UdpSocket* socket, Error error) = 0;

    // Method called when an error occurs during a SendMessage call.
    virtual void OnSendError(UdpSocket* socket, Error error) = 0;

    // Method called when a packet is read.
    virtual void OnRead(UdpSocket* socket, ErrorOr<UdpPacket> packet) = 0;

   protected:
    virtual ~Client();
  };

  // Constants used to specify how we want packets sent from this socket.
  enum class DscpMode : uint8_t {
    // Default value set by the system on creation of a new socket.
    kUnspecified = 0x0,

    // Mode for Audio only.
    kAudioOnly = 0xb8,

    // Mode for Audio + Video.
    kAudioVideo = 0x88,

    // Mode for low priority operations such as trace log data.
    kLowPriority = 0x20
  };

  using Version = IPAddress::Version;

  // Creates a new, scoped UdpSocket within the IPv4 or IPv6 family.
  // |local_endpoint| may be zero (see comments for Bind()). This method must be
  // defined in the platform-level implementation. All |client| methods called
  // will be queued on the provided |task_runner|. For this reason, the provided
  // TaskRunner and Client must exist for the duration of the created socket's
  // lifetime.
  static ErrorOr<std::unique_ptr<UdpSocket>> Create(
      TaskRunner* task_runner,
      Client* client,
      const IPEndpoint& local_endpoint);

  virtual ~UdpSocket();

  // Returns true if |socket| belongs to the IPv4/IPv6 address family.
  virtual bool IsIPv4() const = 0;
  virtual bool IsIPv6() const = 0;

  // Returns the current local endpoint's address and port. Initially, this will
  // be the same as the value that was passed into Create(). However, it can
  // later change after certain operations, such as Bind(), are executed.
  virtual IPEndpoint GetLocalEndpoint() const = 0;

  // Binds to the address specified in the constructor. If the local endpoint's
  // address is zero, the operating system will bind to all interfaces. If the
  // local endpoint's port is zero, the operating system will automatically find
  // a free local port and bind to it. Future calls to GetLocalEndpoint() will
  // reflect the resolved port.
  virtual void Bind() = 0;

  // Sets the device to use for outgoing multicast packets on the socket.
  virtual void SetMulticastOutboundInterface(NetworkInterfaceIndex ifindex) = 0;

  // Joins to the multicast group at the given address, using the specified
  // interface.
  virtual void JoinMulticastGroup(const IPAddress& address,
                                  NetworkInterfaceIndex ifindex) = 0;

  // Sends a message. If the message is not sent, Client::OnSendError() will be
  // called to indicate this. Error::Code::kAgain indicates the operation would
  // block, which can be expected during normal operation.
  virtual void SendMessage(const void* data,
                           size_t length,
                           const IPEndpoint& dest) = 0;

  // Sets the DSCP value to use for all messages sent from this socket.
  virtual void SetDscp(DscpMode state) = 0;

 protected:
  UdpSocket();
};

}  // namespace openscreen

#endif  // PLATFORM_API_UDP_SOCKET_H_
