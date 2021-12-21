// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/udp_packet.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {

TEST(UdpPacketTest, ValidateToStringNormalCase) {
  UdpPacket packet{0x73, 0xC7, 0x00, 0x14, 0xFF, 0x2C};
  std::string result = packet.ToString();
  EXPECT_EQ(result, "[73 C7 00 14 FF 2C ]");

  UdpPacket packet2{0x1, 0x2, 0x3, 0x4, 0x5};
  result = packet2.ToString();
  EXPECT_EQ(result, "[01 02 03 04 05 ]");

  UdpPacket packet3{0x0, 0x0, 0x0};
  result = packet3.ToString();
  EXPECT_EQ(result, "[00 00 00 ]");
}

TEST(UdpPacketTest, ValidateToStringEmpty) {
  UdpPacket packet{};
  std::string result = packet.ToString();
  EXPECT_EQ(result, "[]");
}

}  // namespace openscreen
