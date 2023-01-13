// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/public/dns_sd_instance.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

TEST(DnsSdInstanceTests, InstanceLength) {
  EXPECT_TRUE(IsInstanceValid("instance"));
  EXPECT_TRUE(IsInstanceValid("name"));
  EXPECT_TRUE(IsInstanceValid(""));
  EXPECT_TRUE(IsInstanceValid(
      "Something63CharsLongabcdefghijklmnopqrstuvwxyz1234567890ABCDEFG"));

  EXPECT_FALSE(IsInstanceValid(
      "Something64CharsLongabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGH"));
}

TEST(DnsSdInstanceTests, InstanceCharacters) {
  EXPECT_TRUE(IsInstanceValid("IncludingSpecialCharacters.+ =*&<<+`~\\/"));
  EXPECT_TRUE(IsInstanceValid(".+ =*&<<+`~\\/ "));

  EXPECT_FALSE(IsInstanceValid(std::string(1, uint8_t{0x7F})));
  EXPECT_FALSE(IsInstanceValid(std::string("name with ") +
                               std::string(1, uint8_t{0x7F}) +
                               " in the middle"));
  for (uint8_t bad_char = 0x0; bad_char <= 0x1F; bad_char++) {
    EXPECT_FALSE(IsInstanceValid(std::string(1, bad_char)));
    EXPECT_FALSE(IsInstanceValid(std::string("name with ") +
                                 std::string(1, bad_char) + " in the middle"));
  }
}

TEST(DnsSdInstanceTests, InstanceUTF8) {
  std::vector<uint8_t> char_sets[] = {
      {0x80},
      {0xC0},
      {0xC0, 0xFF},
      {0xE0},
      {0xE0, 0xFF},
      {0xE0, 0x80, 0x00},
      {0xF0},
      {0xF0, 0x00},
      {0xF0, 0x80, 0xFF},
      {0xF0, 0x80, 0x80, 0x0A},
  };

  for (const auto& set : char_sets) {
    std::string test_string = "start";
    for (uint8_t ch : set) {
      test_string.append(std::string(1, ch));
    }

    EXPECT_FALSE(IsInstanceValid(test_string));
  }
}

TEST(DnsSdInstanceTests, ServiceLength) {
  // Too short.
  EXPECT_TRUE(IsServiceValid("_a._udp"));
  EXPECT_FALSE(IsServiceValid("_._udp"));

  // Too long.
  EXPECT_TRUE(IsServiceValid("_abcdefghijklmno._udp"));
  EXPECT_FALSE(IsServiceValid("_abcdefghijklmnop._udp"));
}

TEST(DnsSdInstanceTests, ServiceNonProtocolNameFormatting) {
  EXPECT_TRUE(IsServiceValid("_abcd._udp"));

  // Unexprected protocol string,
  EXPECT_FALSE(IsServiceValid("_abcd._ssl"));
  EXPECT_FALSE(IsServiceValid("_abcd._tls"));
  EXPECT_FALSE(IsServiceValid("_abcd._ucp"));

  // Extra characters before
  EXPECT_FALSE(IsServiceValid(" _abcd._udp"));
  EXPECT_FALSE(IsServiceValid("a_abcd._udp"));
  EXPECT_FALSE(IsServiceValid("-_abcd._udp"));

  // Extra characters after.
  EXPECT_FALSE(IsServiceValid("a_abcd._udp "));
  EXPECT_FALSE(IsServiceValid("a_abcd._udp-"));
  EXPECT_FALSE(IsServiceValid("a_abcd._udpp"));
  EXPECT_FALSE(IsServiceValid("a_abcd._tcp_udp"));
}

TEST(DnsSdInstanceTests, ServiceProtocolNameFormatting) {
  EXPECT_TRUE(IsServiceValid("_abcd._udp"));

  // Disallowed Characters
  EXPECT_FALSE(IsServiceValid("_ab`d._udp"));
  EXPECT_FALSE(IsServiceValid("_a\\cd._udp"));
  EXPECT_FALSE(IsServiceValid("_ab.d._udp"));

  // Contains no letters
  EXPECT_FALSE(IsServiceValid("_123._udp"));
  EXPECT_FALSE(IsServiceValid("_1-3._udp"));

  // Improperly placed hyphen.
  EXPECT_FALSE(IsServiceValid("_-abcd._udp"));
  EXPECT_FALSE(IsServiceValid("_abcd-._udp"));

  // Adjacent hyphens.
  EXPECT_FALSE(IsServiceValid("_abc--d._udp"));
  EXPECT_FALSE(IsServiceValid("_a--bcd._tcp"));
  EXPECT_FALSE(IsServiceValid("_0a1b--c02d._udp"));
  EXPECT_FALSE(IsServiceValid("_0a--1._udp"));
  EXPECT_FALSE(IsServiceValid("_a--b._udp"));

  // Multiple, non-adjacent hyphens.
  EXPECT_TRUE(IsServiceValid("_a-b-c._udp"));
  EXPECT_TRUE(IsServiceValid("_a-1-c._udp"));
}

TEST(DnsSdInstanceTests, DomainDotPositions) {
  EXPECT_TRUE(IsDomainValid("192.168.0.0"));
  EXPECT_TRUE(IsDomainValid("...."));
  EXPECT_TRUE(IsDomainValid(
      "Something63CharsLongabcdefghijklmnopqrstuvwxyz1234567890ABCDEFG."
      "Something63CharsLongabcdefghijklmnopqrstuvwxyz1234567890ABCDEFG"));

  EXPECT_FALSE(IsDomainValid(
      "Something64CharsLongabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGH."
      "Something63CharsLongabcdefghijklmnopqrstuvwxyz1234567890ABCDEFG"));
}

TEST(DnsSdInstanceTests, DomainCharacters) {
  EXPECT_TRUE(IsDomainValid("IncludingSpecialCharacters.+ =*&<<+`~\\/"));
  EXPECT_TRUE(IsDomainValid(".+ =*&<<+`~\\/ "));

  EXPECT_FALSE(IsDomainValid(std::string(1, uint8_t{0x7F})));
  EXPECT_FALSE(IsDomainValid(std::string("name with ") +
                             std::string(1, uint8_t{0x7F}) + " in the middle"));
  for (uint8_t bad_char = 0x0; bad_char <= 0x1F; bad_char++) {
    EXPECT_FALSE(IsDomainValid(std::string(1, bad_char)));
    EXPECT_FALSE(IsDomainValid(std::string("name with ") +
                               std::string(1, bad_char) + " in the middle"));
  }
}

TEST(DnsSdInstanceTests, DomainUTF8) {
  std::vector<uint8_t> char_sets[] = {
      {0x80},
      {0xC0},
      {0xC0, 0xFF},
      {0xE0},
      {0xE0, 0xFF},
      {0xE0, 0x80, 0x00},
      {0xF0},
      {0xF0, 0x00},
      {0xF0, 0x80, 0xFF},
      {0xF0, 0x80, 0x80, 0x0A},
  };

  for (const auto& set : char_sets) {
    std::string test_string = "start";
    for (uint8_t ch : set) {
      test_string.append(std::string(1, ch));
    }

    EXPECT_FALSE(IsDomainValid(test_string));
  }
}

TEST(DnsSdInstanceTests, SubtypeCharacters) {
  EXPECT_TRUE(IsSubtypeValid("IncludingSpecialCharacters+ =*&<<+`~\\/"));
  EXPECT_TRUE(IsSubtypeValid("+ =*&<<+`~\\/ "));

  EXPECT_FALSE(IsSubtypeValid("foo.bar"));
  EXPECT_FALSE(IsSubtypeValid(std::string(1, uint8_t{0x7F})));
  EXPECT_FALSE(IsSubtypeValid(std::string("name with ") +
                              std::string(1, uint8_t{0x7F}) +
                              " in the middle"));
  for (uint8_t bad_char = 0x0; bad_char <= 0x1F; bad_char++) {
    EXPECT_FALSE(IsSubtypeValid(std::string(1, bad_char)));
    EXPECT_FALSE(IsSubtypeValid(std::string("name with ") +
                                std::string(1, bad_char) + " in the middle"));
  }
}

TEST(DnsSdInstanceTests, SubtypeUTF8) {
  // Sets of bytes which do not form valid UTF8 encoded chars.
  std::vector<uint8_t> char_sets[] = {
      {0x80},
      {0xC0},
      {0xC0, 0xFF},
      {0xE0},
      {0xE0, 0xFF},
      {0xE0, 0x80, 0x00},
      {0xF0},
      {0xF0, 0x00},
      {0xF0, 0x80, 0xFF},
      {0xF0, 0x80, 0x80, 0x0A},
  };

  for (const auto& set : char_sets) {
    std::string test_string = "start";
    for (uint8_t ch : set) {
      test_string.append(std::string(1, ch));
    }

    EXPECT_FALSE(IsSubtypeValid(test_string));
  }
}

TEST(DnsSdInstanceTests, SubtypeLength) {
  std::string kCharsAlmostMaxLength =
      "123456989012345678901234567890123456789012345678901234567890123";

  ASSERT_EQ(kCharsAlmostMaxLength.size(), size_t{63});
  EXPECT_TRUE(IsSubtypeValid(kCharsAlmostMaxLength));
  EXPECT_FALSE(IsSubtypeValid(kCharsAlmostMaxLength + "4"));
}

TEST(DnsSdInstanceTests, ComparisonTests) {
  DnsSdTxtRecord set_record;
  set_record.SetValue("foo", "bar");

  DnsSdInstance kIn1("instance", "_service._tcp", "local", {}, 80);
  DnsSdInstance kIn2("instance", "_service._tcp", "local", {}, 80);
  DnsSdInstance kIn3("instance2", "_service._tcp", "local", {}, 80);
  DnsSdInstance kIn4("instance", "_service2._tcp", "local", {}, 80);
  DnsSdInstance kIn5("instance", "_service._tcp", "local2", {}, 80);
  DnsSdInstance kIn6("instance", "_service._tcp", "local", set_record, 80);
  DnsSdInstance kIn7("instance", "_service._tcp", "local", {}, 79);
  DnsSdInstance kIn8("instance", "_service._tcp", "local", {}, 80, "foo");
  DnsSdInstance kIn9("instance", "_service._tcp", "local", {}, 80, "foobar");
  DnsSdInstance kIn10("instance", "_service._tcp", "local", {}, 80, "foo",
                      "bar");

  EXPECT_EQ(kIn1, kIn2);
  EXPECT_LT(kIn1, kIn3);
  EXPECT_LT(kIn1, kIn4);
  EXPECT_LT(kIn1, kIn5);
  EXPECT_LT(kIn1, kIn6);
  EXPECT_GT(kIn1, kIn7);
  EXPECT_LT(kIn1, kIn8);
  EXPECT_LT(kIn1, kIn9);
  EXPECT_LT(kIn1, kIn10);

  EXPECT_LT(kIn8, kIn9);
  EXPECT_LT(kIn8, kIn10);
  EXPECT_LT(kIn9, kIn10);
}

}  // namespace discovery
}  // namespace openscreen
