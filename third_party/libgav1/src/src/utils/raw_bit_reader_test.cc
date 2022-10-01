// Copyright 2021 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/utils/raw_bit_reader.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"
#include "src/utils/constants.h"
#include "tests/third_party/libvpx/acm_random.h"

namespace libgav1 {
namespace {

std::string IntegerToString(int x) { return std::bitset<8>(x).to_string(); }

class RawBitReaderTest : public testing::TestWithParam<std::tuple<int, int>> {
 protected:
  RawBitReaderTest()
      : literal_size_(std::get<0>(GetParam())),
        test_data_size_(std::get<1>(GetParam())) {}

  void CreateReader(const std::vector<uint8_t>& data) {
    data_ = data;
    raw_bit_reader_.reset(new (std::nothrow)
                              RawBitReader(data_.data(), data_.size()));
  }

  void CreateReader(int size) {
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    data_.clear();
    for (int i = 0; i < size; ++i) {
      data_.push_back(rnd.Rand8());
    }
    raw_bit_reader_.reset(new (std::nothrow)
                              RawBitReader(data_.data(), data_.size()));
  }

  // Some tests don't depend on |literal_size_|. For those tests, return true if
  // the |literal_size_| is greater than 1. If this function returns true, the
  // test will abort.
  bool RunOnlyOnce() const { return literal_size_ > 1; }

  std::unique_ptr<RawBitReader> raw_bit_reader_;
  std::vector<uint8_t> data_;
  int literal_size_;
  int test_data_size_;
};

TEST_P(RawBitReaderTest, ReadBit) {
  if (RunOnlyOnce()) return;
  CreateReader(test_data_size_);
  for (const auto& value : data_) {
    const std::string expected = IntegerToString(value);
    for (int j = 0; j < 8; ++j) {
      EXPECT_FALSE(raw_bit_reader_->Finished());
      EXPECT_EQ(static_cast<int>(expected[j] == '1'),
                raw_bit_reader_->ReadBit());
    }
  }
  EXPECT_TRUE(raw_bit_reader_->Finished());
  EXPECT_EQ(raw_bit_reader_->ReadBit(), -1);
}

TEST_P(RawBitReaderTest, ReadLiteral) {
  const int size_bytes = literal_size_;
  const int size_bits = 8 * size_bytes;
  CreateReader(test_data_size_ * size_bytes);
  for (size_t i = 0; i < data_.size(); i += size_bytes) {
    uint32_t expected_literal = 0;
    for (int j = 0; j < size_bytes; ++j) {
      expected_literal |=
          static_cast<uint32_t>(data_[i + j] << (8 * (size_bytes - j - 1)));
    }
    EXPECT_FALSE(raw_bit_reader_->Finished());
    const int64_t actual_literal = raw_bit_reader_->ReadLiteral(size_bits);
    EXPECT_EQ(static_cast<int64_t>(expected_literal), actual_literal);
    EXPECT_GE(actual_literal, 0);
  }
  EXPECT_TRUE(raw_bit_reader_->Finished());
  EXPECT_EQ(raw_bit_reader_->ReadLiteral(10), -1);
}

TEST_P(RawBitReaderTest, ReadLiteral32BitsWithMsbSet) {
  if (RunOnlyOnce()) return;
  // Three 32-bit values with MSB set.
  CreateReader({0xff, 0xff, 0xff, 0xff,    // 4294967295
                0x80, 0xff, 0xee, 0xdd,    // 2164256477
                0xa0, 0xaa, 0xbb, 0xcc});  // 2695543756
  static constexpr int64_t expected_literals[] = {4294967295, 2164256477,
                                                  2695543756};
  for (const int64_t expected_literal : expected_literals) {
    EXPECT_FALSE(raw_bit_reader_->Finished());
    const int64_t actual_literal = raw_bit_reader_->ReadLiteral(32);
    EXPECT_EQ(expected_literal, actual_literal);
    EXPECT_GE(actual_literal, 0);
  }
  EXPECT_TRUE(raw_bit_reader_->Finished());
  EXPECT_EQ(raw_bit_reader_->ReadLiteral(10), -1);
}

TEST_P(RawBitReaderTest, ReadLiteralNotEnoughBits) {
  if (RunOnlyOnce()) return;
  CreateReader(4);  // 32 bits.
  EXPECT_GE(raw_bit_reader_->ReadLiteral(16), 0);
  EXPECT_EQ(raw_bit_reader_->ReadLiteral(32), -1);
}

TEST_P(RawBitReaderTest, ReadLiteralMaxNumBits) {
  if (RunOnlyOnce()) return;
  CreateReader(4);  // 32 bits.
  EXPECT_NE(raw_bit_reader_->ReadLiteral(32), -1);
}

TEST_P(RawBitReaderTest, ReadInverseSignedLiteral) {
  if (RunOnlyOnce()) return;
  // This is the only usage for this function in the decoding process. So
  // testing just that case.
  const int size_bits = 6;
  data_.clear();
  // Negative value followed by a positive value.
  data_.push_back(0xd2);
  data_.push_back(0xa4);
  raw_bit_reader_.reset(new (std::nothrow)
                            RawBitReader(data_.data(), data_.size()));
  int value;
  EXPECT_TRUE(raw_bit_reader_->ReadInverseSignedLiteral(size_bits, &value));
  EXPECT_EQ(value, -23);
  EXPECT_TRUE(raw_bit_reader_->ReadInverseSignedLiteral(size_bits, &value));
  EXPECT_EQ(value, 41);
  // We have only two bits left. Trying to read an inverse signed literal of 2
  // bits actually needs 3 bits. So this should fail.
  EXPECT_FALSE(raw_bit_reader_->ReadInverseSignedLiteral(2, &value));
}

TEST_P(RawBitReaderTest, ZeroSize) {
  if (RunOnlyOnce()) return;
  // Valid data, zero size.
  data_.clear();
  data_.push_back(0xf0);
  raw_bit_reader_.reset(new (std::nothrow) RawBitReader(data_.data(), 0));
  EXPECT_EQ(raw_bit_reader_->ReadBit(), -1);
  EXPECT_EQ(raw_bit_reader_->ReadLiteral(2), -1);
  // NULL data, zero size.
  raw_bit_reader_.reset(new (std::nothrow) RawBitReader(nullptr, 0));
  EXPECT_EQ(raw_bit_reader_->ReadBit(), -1);
  EXPECT_EQ(raw_bit_reader_->ReadLiteral(2), -1);
}

TEST_P(RawBitReaderTest, AlignToNextByte) {
  if (RunOnlyOnce()) return;
  CreateReader({0x00, 0x00, 0x00, 0x0f});
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 0);
  EXPECT_EQ(raw_bit_reader_->byte_offset(), 0);
  EXPECT_TRUE(raw_bit_reader_->AlignToNextByte());
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 0);
  EXPECT_EQ(raw_bit_reader_->byte_offset(), 0);
  EXPECT_NE(raw_bit_reader_->ReadBit(), -1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 1);
  EXPECT_EQ(raw_bit_reader_->byte_offset(), 1);
  EXPECT_TRUE(raw_bit_reader_->AlignToNextByte());
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);
  EXPECT_EQ(raw_bit_reader_->byte_offset(), 1);
  EXPECT_NE(raw_bit_reader_->ReadLiteral(16), -1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 24);
  EXPECT_EQ(raw_bit_reader_->byte_offset(), 3);
  EXPECT_TRUE(raw_bit_reader_->AlignToNextByte());
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 24);
  EXPECT_EQ(raw_bit_reader_->byte_offset(), 3);
  EXPECT_NE(raw_bit_reader_->ReadBit(), -1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 25);
  EXPECT_EQ(raw_bit_reader_->byte_offset(), 4);
  // Some bits are non-zero.
  EXPECT_FALSE(raw_bit_reader_->AlignToNextByte());
}

TEST_P(RawBitReaderTest, VerifyAndSkipTrailingBits) {
  if (RunOnlyOnce()) return;
  std::vector<uint8_t> data;

  // 1 byte trailing byte.
  data.push_back(0x80);
  CreateReader(data);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 0);
  EXPECT_TRUE(raw_bit_reader_->VerifyAndSkipTrailingBits(8));
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);

  // 2 byte trailing byte beginning at a byte-aligned offset.
  data.clear();
  data.push_back(0xf8);
  data.push_back(0x80);
  CreateReader(data);
  EXPECT_NE(raw_bit_reader_->ReadLiteral(8), -1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);
  EXPECT_TRUE(raw_bit_reader_->VerifyAndSkipTrailingBits(8));
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 16);

  // 2 byte trailing byte beginning at a non-byte-aligned offset.
  data.clear();
  data.push_back(0xf8);
  data.push_back(0x00);
  CreateReader(data);
  EXPECT_NE(raw_bit_reader_->ReadLiteral(4), -1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 4);
  EXPECT_TRUE(raw_bit_reader_->VerifyAndSkipTrailingBits(4));
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);

  // Invalid trailing byte at a byte-aligned offset.
  data.clear();
  data.push_back(0xf7);
  data.push_back(0x70);
  CreateReader(data);
  EXPECT_NE(raw_bit_reader_->ReadLiteral(8), -1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);
  EXPECT_FALSE(raw_bit_reader_->VerifyAndSkipTrailingBits(8));

  // Invalid trailing byte at a non-byte-aligned offset.
  CreateReader(data);
  EXPECT_NE(raw_bit_reader_->ReadLiteral(4), -1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 4);
  EXPECT_FALSE(raw_bit_reader_->VerifyAndSkipTrailingBits(12));

  // No more data available.
  CreateReader(data);
  EXPECT_NE(raw_bit_reader_->ReadLiteral(16), -1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 16);
  EXPECT_TRUE(raw_bit_reader_->Finished());
  EXPECT_FALSE(raw_bit_reader_->VerifyAndSkipTrailingBits(8));
}

TEST_P(RawBitReaderTest, ReadLittleEndian) {
  if (RunOnlyOnce()) return;
  std::vector<uint8_t> data;
  size_t actual;

  // Invalid input.
  data.push_back(0x00);  // dummy.
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadLittleEndian(1, nullptr));

  // One byte value.
  data.clear();
  data.push_back(0x01);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadLittleEndian(1, &actual));
  EXPECT_EQ(actual, 1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);
  EXPECT_TRUE(raw_bit_reader_->Finished());

  // One byte value with leading bytes.
  data.clear();
  data.push_back(0x01);
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0x00);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadLittleEndian(4, &actual));
  EXPECT_EQ(actual, 1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 32);
  EXPECT_TRUE(raw_bit_reader_->Finished());

  // Two byte value.
  data.clear();
  data.push_back(0xD9);
  data.push_back(0x01);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadLittleEndian(2, &actual));
  EXPECT_EQ(actual, 473);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 16);
  EXPECT_TRUE(raw_bit_reader_->Finished());

  // Two byte value with leading bytes.
  data.clear();
  data.push_back(0xD9);
  data.push_back(0x01);
  data.push_back(0x00);
  data.push_back(0x00);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadLittleEndian(4, &actual));
  EXPECT_EQ(actual, 473);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 32);
  EXPECT_TRUE(raw_bit_reader_->Finished());

  // Not enough bytes.
  data.clear();
  data.push_back(0x01);
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadLittleEndian(2, &actual));
}

TEST_P(RawBitReaderTest, ReadUnsignedLeb128) {
  if (RunOnlyOnce()) return;
  std::vector<uint8_t> data;
  size_t actual;

  // Invalid input.
  data.push_back(0x00);  // dummy.
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadUnsignedLeb128(nullptr));

  // One byte value.
  data.clear();
  data.push_back(0x01);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadUnsignedLeb128(&actual));
  EXPECT_EQ(actual, 1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);
  EXPECT_TRUE(raw_bit_reader_->Finished());

  // One byte value with trailing bytes.
  data.clear();
  data.push_back(0x81);
  data.push_back(0x80);
  data.push_back(0x80);
  data.push_back(0x00);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadUnsignedLeb128(&actual));
  EXPECT_EQ(actual, 1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 32);
  EXPECT_TRUE(raw_bit_reader_->Finished());

  // Two byte value.
  data.clear();
  data.push_back(0xD9);
  data.push_back(0x01);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadUnsignedLeb128(&actual));
  EXPECT_EQ(actual, 217);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 16);
  EXPECT_TRUE(raw_bit_reader_->Finished());

  // Two byte value with trailing bytes.
  data.clear();
  data.push_back(0xD9);
  data.push_back(0x81);
  data.push_back(0x80);
  data.push_back(0x80);
  data.push_back(0x00);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadUnsignedLeb128(&actual));
  EXPECT_EQ(actual, 217);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 40);
  EXPECT_TRUE(raw_bit_reader_->Finished());

  // Value > 32 bits.
  data.clear();
  for (int i = 0; i < 5; ++i) data.push_back(0xD9);
  data.push_back(0x00);
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadUnsignedLeb128(&actual));

  // Not enough bytes (truncated leb128 value).
  data.clear();
  data.push_back(0x81);
  data.push_back(0x81);
  data.push_back(0x81);
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadUnsignedLeb128(&actual));

  // Exceeds kMaximumLeb128Size.
  data.clear();
  for (int i = 0; i < 10; ++i) data.push_back(0x80);
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadUnsignedLeb128(&actual));
}

TEST_P(RawBitReaderTest, ReadUvlc) {
  if (RunOnlyOnce()) return;
  std::vector<uint8_t> data;
  uint32_t actual;

  // Invalid input.
  data.push_back(0x00);  // dummy.
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadUvlc(nullptr));

  // Zero bit value.
  data.clear();
  data.push_back(0x80);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadUvlc(&actual));
  EXPECT_EQ(actual, 0);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 1);

  // One bit value.
  data.clear();
  data.push_back(0x60);  // 011...
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadUvlc(&actual));
  EXPECT_EQ(actual, 2);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 3);

  // Two bit value.
  data.clear();
  data.push_back(0x38);  // 00111...
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadUvlc(&actual));
  EXPECT_EQ(actual, 6);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 5);

  // 31 bit value.
  data.clear();
  // (1 << 32) - 2 (= UINT32_MAX - 1) is the largest value that can be encoded
  // as uvlc().
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0x01);
  data.push_back(0xFF);
  data.push_back(0xFF);
  data.push_back(0xFF);
  data.push_back(0xFE);
  CreateReader(data);
  ASSERT_TRUE(raw_bit_reader_->ReadUvlc(&actual));
  EXPECT_EQ(actual, UINT32_MAX - 1);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 63);

  // Not enough bits (truncated uvlc value).
  data.clear();
  data.push_back(0x07);
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadUvlc(&actual));

  // 32 bits.
  data.clear();
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0xFF);
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadUvlc(&actual));

  // Exceeds 32 bits.
  data.clear();
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0x00);
  data.push_back(0x0F);
  CreateReader(data);
  EXPECT_FALSE(raw_bit_reader_->ReadUvlc(&actual));
}

TEST_P(RawBitReaderTest, DecodeSignedSubexpWithReference) {
  if (RunOnlyOnce()) return;
  std::vector<uint8_t> data;
  int actual;

  data.push_back(0xa0);  // v = 5;
  CreateReader(data);
  EXPECT_TRUE(raw_bit_reader_->DecodeSignedSubexpWithReference(
      10, 20, 15, kGlobalMotionReadControl, &actual));
  EXPECT_EQ(actual, 12);

  data.clear();
  data.push_back(0xd0);  // v = 6; extra_bit = 1;
  CreateReader(data);
  EXPECT_TRUE(raw_bit_reader_->DecodeSignedSubexpWithReference(
      10, 20, 15, kGlobalMotionReadControl, &actual));
  EXPECT_EQ(actual, 11);

  data.clear();
  data.push_back(0xc8);  // subexp_more_bits = 1; v = 9;
  CreateReader(data);
  EXPECT_TRUE(raw_bit_reader_->DecodeSignedSubexpWithReference(
      10, 40, 15, kGlobalMotionReadControl, &actual));
  EXPECT_EQ(actual, 27);

  data.clear();
  data.push_back(0x60);  // subexp_more_bits = 0; subexp_bits = 6.
  CreateReader(data);
  EXPECT_TRUE(raw_bit_reader_->DecodeSignedSubexpWithReference(
      10, 40, 15, kGlobalMotionReadControl, &actual));
  EXPECT_EQ(actual, 18);

  data.clear();
  data.push_back(0x60);
  CreateReader(data);
  // Control is greater than 32, which makes b >= 32 in DecodeSubexp() and
  // should return false.
  EXPECT_FALSE(raw_bit_reader_->DecodeSignedSubexpWithReference(10, 40, 15, 35,
                                                                &actual));
}

TEST_P(RawBitReaderTest, DecodeUniform) {
  if (RunOnlyOnce()) return;
  // Test the example from the AV1 spec, Section 4.10.7. ns(n).
  // n = 5
  // Value            ns(n) encoding
  // -------------------------------
  // 0                 00
  // 1                 01
  // 2                 10
  // 3                110
  // 4                111
  //
  // The five encoded values are concatenated into two bytes.
  std::vector<uint8_t> data = {0x1b, 0x70};
  CreateReader(data);
  int actual;
  for (int i = 0; i < 5; ++i) {
    EXPECT_TRUE(raw_bit_reader_->DecodeUniform(5, &actual));
    EXPECT_EQ(actual, i);
  }

  // If n is a power of 2, ns(n) is simply the log2(n)-bit representation of
  // the unsigned number.
  // Test n = 16.
  // The 16 encoded values are concatenated into 8 bytes.
  data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
  CreateReader(data);
  for (int i = 0; i < 16; ++i) {
    EXPECT_TRUE(raw_bit_reader_->DecodeUniform(16, &actual));
    EXPECT_EQ(actual, i);
  }
}

TEST_P(RawBitReaderTest, SkipBytes) {
  if (RunOnlyOnce()) return;
  std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00, 0x00};
  CreateReader(data);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 0);
  EXPECT_TRUE(raw_bit_reader_->SkipBytes(1));
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);
  EXPECT_GE(raw_bit_reader_->ReadBit(), 0);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 9);
  EXPECT_FALSE(raw_bit_reader_->SkipBytes(1));  // Not at a byte boundary.
  EXPECT_TRUE(raw_bit_reader_->AlignToNextByte());
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 16);
  EXPECT_FALSE(raw_bit_reader_->SkipBytes(10));  // Not enough bytes.
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 16);
  EXPECT_TRUE(raw_bit_reader_->SkipBytes(3));
  EXPECT_TRUE(raw_bit_reader_->Finished());
  EXPECT_EQ(raw_bit_reader_->ReadBit(), -1);
}

TEST_P(RawBitReaderTest, SkipBits) {
  if (RunOnlyOnce()) return;
  std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00, 0x00};
  CreateReader(data);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 0);
  EXPECT_TRUE(raw_bit_reader_->SkipBits(8));
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 8);
  EXPECT_GE(raw_bit_reader_->ReadBit(), 0);
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 9);
  EXPECT_TRUE(raw_bit_reader_->SkipBits(10));  // Not at a byte boundary.
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 19);
  EXPECT_FALSE(raw_bit_reader_->SkipBits(80));  // Not enough bytes.
  EXPECT_EQ(raw_bit_reader_->bit_offset(), 19);
  EXPECT_TRUE(raw_bit_reader_->SkipBits(21));
  EXPECT_TRUE(raw_bit_reader_->Finished());
  EXPECT_EQ(raw_bit_reader_->ReadBit(), -1);
}

INSTANTIATE_TEST_SUITE_P(
    RawBitReaderTestInstance, RawBitReaderTest,
    testing::Combine(testing::Range(1, 5),    // literal size.
                     testing::Values(100)));  // number of bits/literals.

}  // namespace
}  // namespace libgav1
