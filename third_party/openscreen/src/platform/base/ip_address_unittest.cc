// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/ip_address.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/base/error.h"

namespace openscreen {

using ::testing::ElementsAreArray;

TEST(IPAddressTest, V4Constructors) {
  uint8_t bytes[4] = {};
  IPAddress address1(std::array<uint8_t, 4>{{1, 2, 3, 4}});
  address1.CopyToV4(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({1, 2, 3, 4}));

  uint8_t x[] = {4, 3, 2, 1};
  IPAddress address2(x);
  address2.CopyToV4(bytes);
  EXPECT_THAT(bytes, ElementsAreArray(x));

  const auto b = address2.bytes();
  const uint8_t raw_bytes[4]{b[0], b[1], b[2], b[3]};
  EXPECT_THAT(raw_bytes, ElementsAreArray(x));

  IPAddress address3(IPAddress::Version::kV4, &x[0]);
  address3.CopyToV4(bytes);
  EXPECT_THAT(bytes, ElementsAreArray(x));

  IPAddress address4(6, 5, 7, 9);
  address4.CopyToV4(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({6, 5, 7, 9}));

  IPAddress address5(address4);
  address5.CopyToV4(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({6, 5, 7, 9}));

  address5 = address1;
  address5.CopyToV4(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({1, 2, 3, 4}));
}

TEST(IPAddressTest, V4ComparisonAndBoolean) {
  IPAddress address1;
  EXPECT_EQ(address1, address1);
  EXPECT_FALSE(address1);

  uint8_t x[] = {4, 3, 2, 1};
  IPAddress address2(x);
  EXPECT_NE(address1, address2);
  EXPECT_TRUE(address2);

  IPAddress address3(x);
  EXPECT_EQ(address2, address3);
  EXPECT_TRUE(address3);

  address2 = address1;
  EXPECT_EQ(address1, address2);
  EXPECT_FALSE(address2);
}

TEST(IPAddressTest, V4Parse) {
  uint8_t bytes[4] = {};

  ErrorOr<IPAddress> address = IPAddress::Parse("192.168.0.1");
  ASSERT_TRUE(address);
  address.value().CopyToV4(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({192, 168, 0, 1}));
}

TEST(IPAddressTest, V4ParseFailures) {
  EXPECT_FALSE(IPAddress::Parse("192..0.1"))
      << "empty value should fail to parse";
  EXPECT_FALSE(IPAddress::Parse(".192.168.0.1"))
      << "leading dot should fail to parse";
  EXPECT_FALSE(IPAddress::Parse(".192.168.1"))
      << "leading dot should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("..192.168.0.1"))
      << "leading dot should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("..192.1"))
      << "leading dot should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("192.168.0.1."))
      << "trailing dot should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("192.168.1."))
      << "trailing dot should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("192.168.1.."))
      << "trailing dot should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("192.168.."))
      << "trailing dot should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("192.x3.0.1"))
      << "non-digit character should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("192.3.1"))
      << "too few values should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("192.3.2.0.1"))
      << "too many values should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("1920.3.2.1"))
      << "value > 255 should fail to parse";
}

TEST(IPAddressTest, V6Constructors) {
  uint8_t bytes[16] = {};
  IPAddress address1(std::array<uint16_t, 8>{
      {0x0102, 0x0304, 0x0506, 0x0708, 0x090a, 0x0b0c, 0x0d0e, 0x0f10}});
  address1.CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                       13, 14, 15, 16}));

  const uint8_t x[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  const uint16_t hextets[] = {0x0102, 0x0304, 0x0506, 0x0708,
                              0x090a, 0x0b0c, 0x0d0e, 0x0f10};
  IPAddress address2(hextets);
  address2.CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray(x));

  IPAddress address3(IPAddress::Version::kV6, &x[0]);
  address3.CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray(x));

  IPAddress address4(0x100f, 0x0e0d, 0x0c0b, 0x0a09, 0x0807, 0x0605, 0x0403,
                     0x0201);
  address4.CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6,
                                       5, 4, 3, 2, 1}));

  IPAddress address5(address4);
  address5.CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6,
                                       5, 4, 3, 2, 1}));
}

TEST(IPAddressTest, V6ComparisonAndBoolean) {
  IPAddress address1;
  EXPECT_EQ(address1, address1);
  EXPECT_FALSE(address1);

  uint8_t x[] = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
  IPAddress address2(IPAddress::Version::kV6, x);
  EXPECT_NE(address1, address2);
  EXPECT_TRUE(address2);

  IPAddress address3(IPAddress::Version::kV6, x);
  EXPECT_EQ(address2, address3);
  EXPECT_TRUE(address3);

  address2 = address1;
  EXPECT_EQ(address1, address2);
  EXPECT_FALSE(address2);
}

TEST(IPAddressTest, V6ParseBasic) {
  uint8_t bytes[16] = {};
  ErrorOr<IPAddress> address =
      IPAddress::Parse("abcd:ef01:2345:6789:9876:5432:10FE:DBCA");
  ASSERT_TRUE(address);
  address.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67,
                                       0x89, 0x98, 0x76, 0x54, 0x32, 0x10, 0xfe,
                                       0xdb, 0xca}));
}

TEST(IPAddressTest, V6ParseDoubleColon) {
  uint8_t bytes[16] = {};
  ErrorOr<IPAddress> address1 =
      IPAddress::Parse("abcd:ef01:2345:6789:9876:5432::dbca");
  ASSERT_TRUE(address1);
  address1.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67,
                                       0x89, 0x98, 0x76, 0x54, 0x32, 0x00, 0x00,
                                       0xdb, 0xca}));
  ErrorOr<IPAddress> address2 = IPAddress::Parse("abcd::10fe:dbca");
  ASSERT_TRUE(address2);
  address2.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0xab, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xfe,
                                       0xdb, 0xca}));

  ErrorOr<IPAddress> address3 = IPAddress::Parse("::10fe:dbca");
  ASSERT_TRUE(address3);
  address3.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xfe,
                                       0xdb, 0xca}));

  ErrorOr<IPAddress> address4 = IPAddress::Parse("10fe:dbca::");
  ASSERT_TRUE(address4);
  address4.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0x10, 0xfe, 0xdb, 0xca, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00}));
}

TEST(IPAddressTest, V6SmallValues) {
  uint8_t bytes[16] = {};
  ErrorOr<IPAddress> address1 = IPAddress::Parse("::");
  ASSERT_TRUE(address1);
  address1.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00}));

  ErrorOr<IPAddress> address2 = IPAddress::Parse("::1");
  ASSERT_TRUE(address2);
  address2.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x01}));

  ErrorOr<IPAddress> address3 = IPAddress::Parse("::2:1");
  ASSERT_TRUE(address3);
  address3.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
                                       0x00, 0x01}));
}

TEST(IPAddressTest, V6ParseFailures) {
  EXPECT_FALSE(IPAddress::Parse(":abcd::dbca"))
      << "leading colon should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("abcd::dbca:"))
      << "trailing colon should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("abxd::1234"))
      << "non-hex digit should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("abcd:1234"))
      << "too few values should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("a:b:c:d:e:f:0:1:2:3:4:5:6:7:8:9:a"))
      << "too many values should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("1:2:3:4:5:6:7::8"))
      << "too many values around double-colon should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("1:2:3:4:5:6:7:8::"))
      << "too many values before double-colon should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("::1:2:3:4:5:6:7:8"))
      << "too many values after double-colon should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("abcd1::dbca"))
      << "value > 0xffff should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("::abcd::dbca"))
      << "multiple double colon should fail to parse";

  EXPECT_FALSE(IPAddress::Parse(":::abcd::dbca"))
      << "leading triple colon should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("abcd:::dbca"))
      << "triple colon should fail to parse";
  EXPECT_FALSE(IPAddress::Parse("abcd:dbca:::"))
      << "trailing triple colon should fail to parse";
}

TEST(IPAddressTest, V6ParseThreeDigitValue) {
  uint8_t bytes[16] = {};
  ErrorOr<IPAddress> address = IPAddress::Parse("::123");
  ASSERT_TRUE(address);
  address.value().CopyToV6(bytes);
  EXPECT_THAT(bytes, ElementsAreArray({0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x01, 0x23}));
}

TEST(IPAddressTest, IPEndpointBoolOperator) {
  IPEndpoint endpoint;
  ASSERT_FALSE((endpoint));
  ASSERT_TRUE((IPEndpoint{{192, 168, 0, 1}, 80}));
  ASSERT_TRUE((IPEndpoint{{192, 168, 0, 1}, 0}));
  ASSERT_TRUE((IPEndpoint{{}, 80}));
}

TEST(IPAddressTest, IPEndpointParse) {
  IPEndpoint expected{IPAddress(std::array<uint8_t, 4>{{1, 2, 3, 4}}), 5678};
  ErrorOr<IPEndpoint> result = IPEndpoint::Parse("1.2.3.4:5678");
  ASSERT_TRUE(result.is_value()) << result.error();
  EXPECT_EQ(expected, result.value());

  expected = IPEndpoint{
      IPAddress(std::array<uint16_t, 8>{{0xabcd, 0, 0, 0, 0, 0, 0, 1}}), 99};
  result = IPEndpoint::Parse("[abcd::1]:99");
  ASSERT_TRUE(result.is_value()) << result.error();
  EXPECT_EQ(expected, result.value());

  expected = IPEndpoint{
      IPAddress(std::array<uint16_t, 8>{{0, 0, 0, 0, 0, 0, 0, 0}}), 5791};
  result = IPEndpoint::Parse("[::]:5791");
  ASSERT_TRUE(result.is_value()) << result.error();
  EXPECT_EQ(expected, result.value());

  EXPECT_FALSE(IPEndpoint::Parse(""));              // Empty string.
  EXPECT_FALSE(IPEndpoint::Parse("beef"));          // Random word.
  EXPECT_FALSE(IPEndpoint::Parse("localhost:99"));  // We don't do DNS.
  EXPECT_FALSE(IPEndpoint::Parse(":80"));           // Missing address.
  EXPECT_FALSE(IPEndpoint::Parse("[]:22"));         // Missing address.
  EXPECT_FALSE(IPEndpoint::Parse("1.2.3.4"));       // Missing port after IPv4.
  EXPECT_FALSE(IPEndpoint::Parse("[abcd::1]"));     // Missing port after IPv6.
  EXPECT_FALSE(IPEndpoint::Parse("abcd::1:8080"));  // Missing square brackets.

  // No extra whitespace is allowed.
  EXPECT_FALSE(IPEndpoint::Parse(" 1.2.3.4:5678"));
  EXPECT_FALSE(IPEndpoint::Parse("1.2.3.4 :5678"));
  EXPECT_FALSE(IPEndpoint::Parse("1.2.3.4: 5678"));
  EXPECT_FALSE(IPEndpoint::Parse("1.2.3.4:5678 "));
  EXPECT_FALSE(IPEndpoint::Parse(" [abcd::1]:99"));
  EXPECT_FALSE(IPEndpoint::Parse("[abcd::1] :99"));
  EXPECT_FALSE(IPEndpoint::Parse("[abcd::1]: 99"));
  EXPECT_FALSE(IPEndpoint::Parse("[abcd::1]:99 "));
}

TEST(IPAddressTest, IPAddressComparisons) {
  const IPAddress kV4Low{192, 168, 0, 1};
  const IPAddress kV4High{192, 168, 0, 2};
  const IPAddress kV6Low{0, 0, 0, 0, 0, 0, 0, 1};
  const IPAddress kV6High{0, 0, 1, 0, 0, 0, 0, 0};

  EXPECT_TRUE(kV4Low == kV4Low);
  EXPECT_TRUE(kV4High == kV4High);
  EXPECT_TRUE(kV6Low == kV6Low);
  EXPECT_TRUE(kV6High == kV6High);
  EXPECT_FALSE(kV4Low == kV4High);
  EXPECT_FALSE(kV4High == kV4Low);
  EXPECT_FALSE(kV6Low == kV6High);
  EXPECT_FALSE(kV6High == kV6Low);

  EXPECT_FALSE(kV4Low != kV4Low);
  EXPECT_FALSE(kV4High != kV4High);
  EXPECT_FALSE(kV6Low != kV6Low);
  EXPECT_FALSE(kV6High != kV6High);
  EXPECT_TRUE(kV4Low != kV4High);
  EXPECT_TRUE(kV4High != kV4Low);
  EXPECT_TRUE(kV6Low != kV6High);
  EXPECT_TRUE(kV6High != kV6Low);

  EXPECT_TRUE(kV4Low < kV4High);
  EXPECT_TRUE(kV4High < kV6Low);
  EXPECT_TRUE(kV6Low < kV6High);
  EXPECT_FALSE(kV6High < kV6Low);
  EXPECT_FALSE(kV6Low < kV4High);
  EXPECT_FALSE(kV4High < kV4Low);

  EXPECT_FALSE(kV4Low > kV4High);
  EXPECT_FALSE(kV4High > kV6Low);
  EXPECT_FALSE(kV6Low > kV6High);
  EXPECT_TRUE(kV6High > kV6Low);
  EXPECT_TRUE(kV6Low > kV4High);
  EXPECT_TRUE(kV4High > kV4Low);

  EXPECT_TRUE(kV4Low <= kV4High);
  EXPECT_TRUE(kV4High <= kV6Low);
  EXPECT_TRUE(kV6Low <= kV6High);
  EXPECT_TRUE(kV4Low <= kV4Low);
  EXPECT_TRUE(kV4High <= kV4High);
  EXPECT_TRUE(kV6Low <= kV6Low);
  EXPECT_TRUE(kV6High <= kV6High);
  EXPECT_FALSE(kV6High <= kV6Low);
  EXPECT_FALSE(kV6Low <= kV4High);
  EXPECT_FALSE(kV4High <= kV4Low);

  EXPECT_FALSE(kV4Low >= kV4High);
  EXPECT_FALSE(kV4High >= kV6Low);
  EXPECT_FALSE(kV6Low >= kV6High);
  EXPECT_TRUE(kV4Low >= kV4Low);
  EXPECT_TRUE(kV4High >= kV4High);
  EXPECT_TRUE(kV6Low >= kV6Low);
  EXPECT_TRUE(kV6High >= kV6High);
  EXPECT_TRUE(kV6High >= kV6Low);
  EXPECT_TRUE(kV6Low >= kV4High);
  EXPECT_TRUE(kV4High >= kV4Low);
}

TEST(IPAddressTest, IPEndpointComparisons) {
  const IPEndpoint kV4LowHighPort{{192, 168, 0, 1}, 1000};
  const IPEndpoint kV4LowLowPort{{192, 168, 0, 1}, 1};
  const IPEndpoint kV4High{{192, 168, 0, 2}, 22};
  const IPEndpoint kV6Low{{0, 0, 0, 0, 0, 0, 0, 1}, 22};
  const IPEndpoint kV6High{{0, 0, 1, 0, 0, 0, 0, 0}, 22};

  EXPECT_TRUE(kV4LowHighPort == kV4LowHighPort);
  EXPECT_TRUE(kV4High == kV4High);
  EXPECT_TRUE(kV6Low == kV6Low);
  EXPECT_TRUE(kV6High == kV6High);

  EXPECT_TRUE(kV4LowLowPort != kV4LowHighPort);
  EXPECT_TRUE(kV4LowLowPort != kV4High);
  EXPECT_TRUE(kV4High != kV6Low);
  EXPECT_TRUE(kV6Low != kV6High);

  EXPECT_TRUE(kV4LowLowPort < kV4LowHighPort);
  EXPECT_TRUE(kV4LowLowPort < kV4High);
  EXPECT_TRUE(kV4High < kV6Low);
  EXPECT_TRUE(kV6Low < kV6High);

  EXPECT_TRUE(kV4LowHighPort > kV4LowLowPort);
  EXPECT_TRUE(kV4High > kV4LowLowPort);
  EXPECT_TRUE(kV6Low > kV4High);
  EXPECT_TRUE(kV6High > kV6Low);

  EXPECT_TRUE(kV4LowLowPort <= kV4LowHighPort);
  EXPECT_TRUE(kV4LowLowPort <= kV4High);
  EXPECT_TRUE(kV4High <= kV6Low);
  EXPECT_TRUE(kV6Low <= kV6High);
  EXPECT_TRUE(kV4LowLowPort <= kV4LowHighPort);
  EXPECT_TRUE(kV4LowLowPort <= kV4High);
  EXPECT_TRUE(kV4High <= kV6Low);
  EXPECT_TRUE(kV6Low <= kV6High);

  EXPECT_FALSE(kV4LowLowPort >= kV4LowHighPort);
  EXPECT_FALSE(kV4LowLowPort >= kV4High);
  EXPECT_FALSE(kV4High >= kV6Low);
  EXPECT_FALSE(kV6Low >= kV6High);
  EXPECT_TRUE(kV4LowHighPort >= kV4LowLowPort);
  EXPECT_TRUE(kV4High >= kV4LowLowPort);
  EXPECT_TRUE(kV6Low >= kV4High);
  EXPECT_TRUE(kV6High >= kV6Low);
  EXPECT_TRUE(kV4LowHighPort >= kV4LowLowPort);
  EXPECT_TRUE(kV4High >= kV4LowLowPort);
  EXPECT_TRUE(kV6Low >= kV4High);
  EXPECT_TRUE(kV6High >= kV6Low);
}

TEST(IPAddressTest, OstreamOperatorForIPv4) {
  std::ostringstream oss;
  oss << IPAddress{192, 168, 1, 2};
  EXPECT_EQ("192.168.1.2", oss.str());

  oss.str("");
  oss << IPAddress{192, 168, 0, 2};
  EXPECT_EQ("192.168.0.2", oss.str());

  oss.str("");
  oss << IPAddress{23, 45, 67, 89};
  EXPECT_EQ("23.45.67.89", oss.str());
}

}  // namespace openscreen
