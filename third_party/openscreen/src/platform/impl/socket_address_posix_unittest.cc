// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/socket_address_posix.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {

TEST(SocketAddressPosixTest, IPv4SocketAddressConvertsSuccessfully) {
  const SocketAddressPosix address(IPEndpoint{{1, 2, 3, 4}, 80});

  const sockaddr_in* v4_address =
      reinterpret_cast<const sockaddr_in*>(address.address());

  EXPECT_EQ(v4_address->sin_family, AF_INET);
  EXPECT_EQ(v4_address->sin_port, ntohs(80));

  // 67305985 == 1.2.3.4 in network byte order
  EXPECT_EQ(v4_address->sin_addr.s_addr, 67305985u);

  EXPECT_EQ(address.version(), IPAddress::Version::kV4);
}

TEST(SocketAddressPosixTest, IPv6SocketAddressConvertsSuccessfully) {
  const SocketAddressPosix address(IPEndpoint{
      {0x0102, 0x0304, 0x0506, 0x0708, 0x090a, 0x0b0c, 0x0d0e, 0x0f10}, 80});

  const sockaddr_in6* v6_address =
      reinterpret_cast<const sockaddr_in6*>(address.address());
  EXPECT_EQ(v6_address->sin6_family, AF_INET6);
  EXPECT_EQ(v6_address->sin6_port, ntohs(80));
  EXPECT_EQ(v6_address->sin6_flowinfo, 0u);

  const unsigned char kExpectedAddress[16] = {1, 2,  3,  4,  5,  6,  7,  8,
                                              9, 10, 11, 12, 13, 14, 15, 16};
  EXPECT_THAT(v6_address->sin6_addr.s6_addr,
              testing::ElementsAreArray(kExpectedAddress));
  EXPECT_EQ(v6_address->sin6_scope_id, 0u);

  EXPECT_EQ(address.version(), IPAddress::Version::kV6);
}

TEST(SocketAddressPosixTest, IPv4ConvertsSuccessfully) {
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = ntohs(80);
  address.sin_addr.s_addr = 67305985u;

  struct sockaddr* casted = reinterpret_cast<struct sockaddr*>(&address);
  SocketAddressPosix address_posix(*casted);
  const sockaddr_in* v4_address =
      reinterpret_cast<const sockaddr_in*>(address_posix.address());

  EXPECT_EQ(v4_address->sin_family, address.sin_family);
  EXPECT_EQ(v4_address->sin_port, address.sin_port);
  EXPECT_EQ(v4_address->sin_addr.s_addr, address.sin_addr.s_addr);
  IPEndpoint expected_address{{1, 2, 3, 4}, 80};
  EXPECT_EQ(address_posix.endpoint(), expected_address);
}

TEST(SocketAddressPosixTest, IPv6ConvertsSuccessfully) {
  const unsigned char kExpectedAddress[16] = {1, 2,  3,  4,  5,  6,  7,  8,
                                              9, 10, 11, 12, 13, 14, 15, 16};

  struct sockaddr_in6 address;
  address.sin6_family = AF_INET6;
  address.sin6_port = ntohs(80);
  address.sin6_flowinfo = 0u;
  address.sin6_scope_id = 0u;
  memcpy(&address.sin6_addr.s6_addr, kExpectedAddress, 16);

  struct sockaddr* casted = reinterpret_cast<struct sockaddr*>(&address);
  SocketAddressPosix address_posix(*casted);
  const sockaddr_in6* v6_address =
      reinterpret_cast<const sockaddr_in6*>(address_posix.address());

  EXPECT_EQ(v6_address->sin6_family, address.sin6_family);
  EXPECT_EQ(v6_address->sin6_port, address.sin6_port);
  EXPECT_EQ(v6_address->sin6_flowinfo, address.sin6_flowinfo);
  EXPECT_EQ(v6_address->sin6_scope_id, address.sin6_scope_id);
  EXPECT_THAT(v6_address->sin6_addr.s6_addr,
              testing::ElementsAreArray(kExpectedAddress));
  IPEndpoint expected_address{
      {0x0102, 0x0304, 0x0506, 0x0708, 0x090a, 0x0b0c, 0x0d0e, 0x0f10}, 80};
  EXPECT_EQ(address_posix.endpoint(), expected_address);
}

}  // namespace openscreen
