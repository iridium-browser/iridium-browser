// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_UDP_PACKET_H_
#define PLATFORM_BASE_UDP_PACKET_H_

#include <stdint.h>

#include <string>
#include <utility>
#include <vector>

#include "platform/base/ip_address.h"

namespace openscreen {

class UdpSocket;

// A move-only std::vector of bytes that may not exceed the maximum possible
// size of a UDP packet. Implicit copy construction/assignment is disabled to
// prevent hidden copies (i.e., those not explicitly coded).
class UdpPacket : public std::vector<uint8_t> {
 public:
  // C++14 vector constructors, sans Allocator foo, and no copy ctor.
  UdpPacket();
  explicit UdpPacket(size_type size, uint8_t fill_value = {});
  template <typename InputIt>
  UdpPacket(InputIt first, InputIt last) : std::vector<uint8_t>(first, last) {}
  UdpPacket(UdpPacket&& other) noexcept;
  UdpPacket(std::initializer_list<uint8_t> init);

  ~UdpPacket();

  UdpPacket& operator=(UdpPacket&& other);

  const IPEndpoint& source() const { return source_; }
  void set_source(IPEndpoint endpoint) { source_ = std::move(endpoint); }

  const IPEndpoint& destination() const { return destination_; }
  void set_destination(IPEndpoint endpoint) {
    destination_ = std::move(endpoint);
  }

  UdpSocket* socket() const { return socket_; }
  void set_socket(UdpSocket* socket) { socket_ = socket; }

  std::string ToString() const;

  static const size_type kUdpMaxPacketSize;

 private:
  IPEndpoint source_ = {};
  IPEndpoint destination_ = {};
  UdpSocket* socket_ = nullptr;

  OSP_DISALLOW_COPY_AND_ASSIGN(UdpPacket);
};

}  // namespace openscreen

#endif  // PLATFORM_BASE_UDP_PACKET_H_
