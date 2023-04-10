// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_ENVIRONMENT_H_
#define CAST_STREAMING_ENVIRONMENT_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <vector>

#include "platform/api/time.h"
#include "platform/api/udp_socket.h"
#include "platform/base/ip_address.h"
#include "platform/base/span.h"

namespace openscreen {
namespace cast {

// Provides the common environment for operating system resources shared by
// multiple components.
class Environment : public UdpSocket::Client {
 public:
  class PacketConsumer {
   public:
    virtual void OnReceivedPacket(const IPEndpoint& source,
                                  Clock::time_point arrival_time,
                                  std::vector<uint8_t> packet) = 0;

   protected:
    virtual ~PacketConsumer();
  };

  // Consumers of the environment's UDP socket should be careful to check the
  // socket's state before accessing its methods, especially
  // GetBoundLocalEndpoint(). If the environment is |kStarting|, the
  // local endpoint may not be set yet and will be zero initialized.
  enum class SocketState {
    // Socket is still initializing. Usually the UDP socket bind is
    // the last piece.
    kStarting,

    // The socket is ready for use and has been bound.
    kReady,

    // The socket is either closed (normally or due to an error) or in an
    // invalid state. Currently the environment does not create a new socket
    // in this case, so to be used again the environment itself needs to be
    // recreated.
    kInvalid
  };

  // Classes concerned with the Environment's UDP socket state may inherit from
  // |Subscriber| and then |Subscribe|.
  class SocketSubscriber {
   public:
    // Event that occurs when the environment is ready for use.
    virtual void OnSocketReady() = 0;

    // Event that occurs when the environment has experienced a fatal error.
    virtual void OnSocketInvalid(Error error) = 0;

   protected:
    virtual ~SocketSubscriber();
  };

  // Construct with the given clock source and TaskRunner. Creates and
  // internally-owns a UdpSocket, and immediately binds it to the given
  // |local_endpoint|. Default behavior if |local_endpoint| is omitted is to
  // bind to all available interfaces using IPv4.
  Environment(ClockNowFunctionPtr now_function,
              TaskRunner* task_runner,
              const IPEndpoint& local_endpoint = IPEndpoint::kAnyV4());

  ~Environment() override;

  ClockNowFunctionPtr now_function() const { return now_function_; }
  Clock::time_point now() const { return now_function_(); }
  TaskRunner* task_runner() const { return task_runner_; }

  // Returns the local endpoint the socket is bound to, or the zero IPEndpoint
  // if socket creation/binding failed.
  //
  // Note: This method is virtual to allow unit tests to fake that there really
  // is a bound socket.
  virtual IPEndpoint GetBoundLocalEndpoint() const;

  // Get/Set the remote endpoint. This is separate from the constructor because
  // the remote endpoint is, in some cases, discovered only after receiving a
  // packet.
  const IPEndpoint& remote_endpoint() const { return remote_endpoint_; }
  void set_remote_endpoint(const IPEndpoint& endpoint) {
    remote_endpoint_ = endpoint;
  }

  SocketState socket_state() const { return state_; }
  void SetSocketStateForTesting(SocketState state);

  // Subscribe to socket changes. Callers can unsubscribe by passing
  // nullptr.
  void SetSocketSubscriber(SocketSubscriber* subscriber);

  // Start/Resume delivery of incoming packets to the given |packet_consumer|.
  // Delivery will continue until DropIncomingPackets() is called.
  void ConsumeIncomingPackets(PacketConsumer* packet_consumer);

  // Stop delivery of incoming packets, dropping any that do come in. All
  // internal references to the PacketConsumer that was provided in the last
  // call to ConsumeIncomingPackets() are cleared.
  void DropIncomingPackets();

  // Returns the maximum packet size for the network. This will always return a
  // value of at least kRequiredNetworkPacketSize.
  int GetMaxPacketSize() const;

  // Sends the given |packet| to the remote endpoint, best-effort.
  // set_remote_endpoint() must be called beforehand with a valid IPEndpoint.
  //
  // Note: This method is virtual to allow unit tests to intercept packets
  // before they actually head-out through the socket.
  virtual void SendPacket(ByteView packet);

 protected:
  Environment() : now_function_(nullptr), task_runner_(nullptr) {}

  // Protected so that they can be set by the MockEnvironment for testing.
  ClockNowFunctionPtr now_function_;
  TaskRunner* task_runner_;

 private:
  // UdpSocket::Client implementation.
  void OnBound(UdpSocket* socket) final;
  void OnError(UdpSocket* socket, Error error) final;
  void OnSendError(UdpSocket* socket, Error error) final;
  void OnRead(UdpSocket* socket, ErrorOr<UdpPacket> packet_or_error) final;

  // The UDP socket bound to the local endpoint that was passed into the
  // constructor, or null if socket creation failed.
  const std::unique_ptr<UdpSocket> socket_;

  // These are externally set/cleared. Behaviors are described in getter/setter
  // method comments above.
  IPEndpoint local_endpoint_{};
  IPEndpoint remote_endpoint_{};
  PacketConsumer* packet_consumer_ = nullptr;
  SocketState state_ = SocketState::kStarting;
  SocketSubscriber* socket_subscriber_ = nullptr;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_ENVIRONMENT_H_
