// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_encoding.h"

#include <vector>

#include <gtest/gtest.h>

namespace ipp {

namespace {

// Represents single test case.
struct TestCase {
  // a value to read/write
  int64_t value;
  // 1-,2- and 4- bytes representations of the value in two's-complement binary
  // encoding. Corresponding vector is empty <=> the value is out of range
  // (cannot be saved on given number of bytes).
  std::vector<uint8_t> as1byte;
  std::vector<uint8_t> as2bytes;
  std::vector<uint8_t> as4bytes;
  // constructor
  explicit TestCase(int64_t v) : value(v) {}
};

// All Test* templates below have the same parameters:
// BytesCount - 1, 2 or 4 - designated integer size in binary encoding
// Integer / SignedInteger / UnsignedInteger - type of parameter used in
//   instantiation of tested template
// binary - one of the buffer from TestCase structure
// value - a value from TestCase structure

template <size_t BytesCount, typename Integer>
void TestReadSignedInt(const std::vector<uint8_t>& binary,
                       const int64_t value) {
  const uint8_t* ptr = binary.data();
  Integer out;
  ParseSignedInteger<BytesCount, Integer>(&ptr, &out);
  EXPECT_EQ(ptr, binary.data() + BytesCount);
  EXPECT_EQ(out, value);
}

template <size_t BytesCount, typename Integer>
void TestReadUnsignedInt(const std::vector<uint8_t>& binary,
                         const int64_t value) {
  const uint8_t* ptr = binary.data();
  Integer out = 123;
  const bool result = ParseUnsignedInteger<BytesCount, Integer>(&ptr, &out);
  EXPECT_EQ(ptr, binary.data() + BytesCount);
  if (value < 0) {
    EXPECT_FALSE(result);
    EXPECT_EQ(out, 123);
  } else {
    EXPECT_TRUE(result);
    EXPECT_EQ(out, value);
  }
}

template <size_t BytesCount, typename SignedInteger, typename UnsignedInteger>
void TestRead(const std::vector<uint8_t>& binary, const int64_t value) {
  static_assert(std::is_integral<SignedInteger>::value, "integral expected");
  static_assert(std::is_integral<UnsignedInteger>::value, "integral expected");
  static_assert(std::is_signed<SignedInteger>::value, "signed expected");
  static_assert(std::is_unsigned<UnsignedInteger>::value, "unsigned expected");
  static_assert(sizeof(SignedInteger) == sizeof(UnsignedInteger),
                "the same sizes expected");
  if (!binary.empty()) {
    ASSERT_EQ(BytesCount, binary.size());
    TestReadSignedInt<BytesCount, SignedInteger>(binary, value);
    TestReadUnsignedInt<BytesCount, SignedInteger>(binary, value);
    TestReadUnsignedInt<BytesCount, UnsignedInteger>(binary, value);
  }
}

template <size_t BytesCount, typename Integer>
void TestWriteInt(const std::vector<uint8_t>& binary, const Integer value) {
  std::vector<uint8_t> buffer(BytesCount, 123);
  uint8_t* ptr = buffer.data();
  const bool result = WriteInteger<BytesCount, Integer>(&ptr, value);
  if (binary.empty()) {
    EXPECT_FALSE(result);
    EXPECT_EQ(ptr, buffer.data());
    EXPECT_EQ(buffer, std::vector<uint8_t>(BytesCount, 123));
  } else {
    EXPECT_TRUE(result);
    EXPECT_EQ(ptr, buffer.data() + BytesCount);
    EXPECT_EQ(buffer, binary);
  }
}

template <size_t BytesCount, typename SignedInteger, typename UnsignedInteger>
void TestWrite(const std::vector<uint8_t>& binary, const int64_t value) {
  static_assert(std::is_integral<SignedInteger>::value, "integral expected");
  static_assert(std::is_integral<UnsignedInteger>::value, "integral expected");
  static_assert(std::is_signed<SignedInteger>::value, "signed expected");
  static_assert(std::is_unsigned<UnsignedInteger>::value, "unsigned expected");
  static_assert(sizeof(SignedInteger) == sizeof(UnsignedInteger),
                "the same sizes expected");
  ASSERT_TRUE(BytesCount == binary.size() || binary.empty());
  if ((value >= std::numeric_limits<SignedInteger>::min()) &&
      (value <= std::numeric_limits<SignedInteger>::max())) {
    TestWriteInt<BytesCount, SignedInteger>(binary, value);
  }
  if ((value >= 0) && (value <= std::numeric_limits<UnsignedInteger>::max())) {
    TestWriteInt<BytesCount, UnsignedInteger>(binary, value);
  }
}

// Runs all possible tests for given TestCase.
void TestReadAndWrite(const TestCase& tc) {
  // test read (only correct types are allowed)
  TestRead<1, int8_t, uint8_t>(tc.as1byte, tc.value);
  TestRead<1, int16_t, uint16_t>(tc.as1byte, tc.value);
  TestRead<1, int32_t, uint32_t>(tc.as1byte, tc.value);
  TestRead<2, int16_t, uint16_t>(tc.as2bytes, tc.value);
  TestRead<2, int32_t, uint32_t>(tc.as2bytes, tc.value);
  TestRead<4, int32_t, uint32_t>(tc.as4bytes, tc.value);
  // test write
  TestWrite<1, int8_t, uint8_t>(tc.as1byte, tc.value);
  TestWrite<1, int16_t, uint16_t>(tc.as1byte, tc.value);
  TestWrite<1, int32_t, uint32_t>(tc.as1byte, tc.value);
  TestWrite<2, int8_t, uint8_t>(tc.as2bytes, tc.value);
  TestWrite<2, int16_t, uint16_t>(tc.as2bytes, tc.value);
  TestWrite<2, int32_t, uint32_t>(tc.as2bytes, tc.value);
  TestWrite<4, int8_t, uint8_t>(tc.as4bytes, tc.value);
  TestWrite<4, int16_t, uint16_t>(tc.as4bytes, tc.value);
  TestWrite<4, int32_t, uint32_t>(tc.as4bytes, tc.value);
}

TEST(encoding, TwosComplementary0) {
  TestCase tc(0);
  tc.as1byte.resize(1, 0);
  tc.as2bytes.resize(2, 0);
  tc.as4bytes.resize(4, 0);
  TestReadAndWrite(tc);
}

// min 1-byte int
TEST(encoding, TwosComplementaryNeg128) {
  TestCase tc(-128);
  tc.as1byte = {0x80};
  tc.as2bytes = {0xff, 0x80};
  tc.as4bytes = {0xff, 0xff, 0xff, 0x80};
  TestReadAndWrite(tc);
}

// min 2-bytes int
TEST(encoding, TwosComplementaryNeg32768) {
  TestCase tc(-32768);
  tc.as2bytes = {0x80, 0x00};
  tc.as4bytes = {0xff, 0xff, 0x80, 0x00};
  TestReadAndWrite(tc);
}

// min 4-bytes int
TEST(encoding, TwosComplementaryNeg2147483648) {
  TestCase tc(-2147483648);
  tc.as4bytes = {0x80, 0x00, 0x00, 0x00};
  TestReadAndWrite(tc);
}

// max 1-byte int
TEST(encoding, TwosComplementary127) {
  TestCase tc(127);
  tc.as1byte = {0x7f};
  tc.as2bytes = {0x00, 0x7f};
  tc.as4bytes = {0x00, 0x00, 0x00, 0x7f};
  TestReadAndWrite(tc);
}

// max 2-bytes int
TEST(encoding, TwosComplementary32767) {
  TestCase tc(32767);
  tc.as2bytes = {0x7f, 0xff};
  tc.as4bytes = {0x00, 0x00, 0x7f, 0xff};
  TestReadAndWrite(tc);
}

// max 4-bytes int
TEST(encoding, TwosComplementary2147483647) {
  TestCase tc(2147483647);
  tc.as4bytes = {0x7f, 0xff, 0xff, 0xff};
  TestReadAndWrite(tc);
}

}  // end of namespace

}  // end of namespace ipp
