// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/udp_packet.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/stringprintf.h"

namespace openscreen {

TEST(UdpPacketTest, ValidateToStringNormalCase) {
  UdpPacket packet{0x73, 0xC7, 0x00, 0x14, 0xFF, 0x2C};
  std::string result = HexEncode(packet);
  EXPECT_EQ(result, "73c70014ff2c");

  UdpPacket packet2{0x1, 0x2, 0x3, 0x4, 0x5};
  result = HexEncode(packet2);
  EXPECT_EQ(result, "0102030405");

  UdpPacket packet3{0x0, 0x0, 0x0};
  result = HexEncode(packet3);
  EXPECT_EQ(result, "000000");
}

TEST(UdpPacketTest, ValidateToStringEmpty) {
  UdpPacket packet{};
  std::string result = HexEncode(packet);
  EXPECT_EQ(result, "");
}

}  // namespace openscreen
