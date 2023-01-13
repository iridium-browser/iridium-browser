// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/tls_write_buffer.h"

#include <algorithm>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace {

TEST(TlsWriteBufferTest, CheckBasicFunctionality) {
  TlsWriteBuffer buffer;
  constexpr size_t write_size = TlsWriteBuffer::kBufferSizeBytes / 2;
  uint8_t write_buffer[write_size];
  std::fill_n(write_buffer, write_size, uint8_t{1});

  EXPECT_TRUE(buffer.Push(write_buffer, write_size));

  absl::Span<const uint8_t> readable_data = buffer.GetReadableRegion();
  ASSERT_EQ(readable_data.size(), write_size);
  for (size_t i = 0; i < readable_data.size(); i++) {
    EXPECT_EQ(readable_data[i], 1);
  }

  buffer.Consume(write_size / 2);

  readable_data = buffer.GetReadableRegion();
  ASSERT_EQ(readable_data.size(), write_size / 2);
  for (size_t i = 0; i < readable_data.size(); i++) {
    EXPECT_EQ(readable_data[i], 1);
  }

  buffer.Consume(write_size / 2);

  readable_data = buffer.GetReadableRegion();
  ASSERT_EQ(readable_data.size(), size_t{0});

  // Test that the entire buffer can be used.
  EXPECT_TRUE(buffer.Push(write_buffer, write_size));
  EXPECT_TRUE(buffer.Push(write_buffer, write_size));
  // The buffer should be 100% full at this point. Confirm that no more can be
  // written.
  EXPECT_FALSE(buffer.Push(write_buffer, write_size));
  EXPECT_FALSE(buffer.Push(write_buffer, 1));
}

TEST(TlsWriteBufferTest, TestWrapAround) {
  TlsWriteBuffer buffer;
  constexpr size_t buffer_size = TlsWriteBuffer::kBufferSizeBytes;
  uint8_t write_buffer[buffer_size];
  std::fill_n(write_buffer, buffer_size, uint8_t{1});

  constexpr size_t partial_buffer_size = buffer_size * 3 / 4;
  EXPECT_TRUE(buffer.Push(write_buffer, partial_buffer_size));
  // Buffer contents should now be: |111111111111····|
  auto region = buffer.GetReadableRegion();
  auto* const buffer_begin = region.data();
  ASSERT_TRUE(buffer_begin);
  EXPECT_EQ(region.size(), partial_buffer_size);
  EXPECT_TRUE(std::all_of(region.begin(), region.end(),
                          [](uint8_t byte) { return byte == 1; }));

  buffer.Consume(buffer_size / 2);
  // Buffer contents should now be: |········1111····|
  region = buffer.GetReadableRegion();
  EXPECT_EQ(region.data(), buffer_begin + buffer_size / 2);
  EXPECT_EQ(region.size(), buffer_size / 4);
  EXPECT_TRUE(std::all_of(region.begin(), region.end(),
                          [](uint8_t byte) { return byte == 1; }));

  std::fill_n(write_buffer, buffer_size, uint8_t{2});
  EXPECT_TRUE(buffer.Push(write_buffer, buffer_size / 2));
  // Buffer contents should now be: |2222····11112222|
  // Readable region should just be the end part.
  region = buffer.GetReadableRegion();
  EXPECT_EQ(region.data(), buffer_begin + buffer_size / 2);
  EXPECT_EQ(region.size(), buffer_size / 2);
  EXPECT_TRUE(std::all_of(region.begin(), region.begin() + buffer_size / 4,
                          [](uint8_t byte) { return byte == 1; }));
  EXPECT_TRUE(std::all_of(region.begin() + buffer_size / 4, region.end(),
                          [](uint8_t byte) { return byte == 2; }));

  buffer.Consume(buffer_size / 2);
  // Buffer contents should now be: |2222············|
  region = buffer.GetReadableRegion();
  EXPECT_EQ(region.data(), buffer_begin);
  EXPECT_EQ(region.size(), buffer_size / 4);
  EXPECT_TRUE(std::all_of(region.begin(), region.end(),
                          [](uint8_t byte) { return byte == 2; }));

  std::fill_n(write_buffer, buffer_size, uint8_t{3});
  // The following Push() fails (not enough room).
  EXPECT_FALSE(buffer.Push(write_buffer, buffer_size));
  // Buffer contents should still be: |2222············|
  EXPECT_TRUE(buffer.Push(write_buffer, buffer_size * 3 / 4));
  // Buffer contents should now be: |2222333333333333|
  EXPECT_FALSE(buffer.Push(write_buffer, buffer_size));  // Not enough room.
  EXPECT_FALSE(buffer.Push(write_buffer, 1));            // Not enough room.
  region = buffer.GetReadableRegion();
  EXPECT_EQ(region.data(), buffer_begin);
  EXPECT_EQ(region.size(), buffer_size);
  EXPECT_TRUE(std::all_of(region.begin(), region.begin() + buffer_size / 4,
                          [](uint8_t byte) { return byte == 2; }));
  EXPECT_TRUE(std::all_of(region.begin() + buffer_size / 4, region.end(),
                          [](uint8_t byte) { return byte == 3; }));

  buffer.Consume(buffer_size);
  // Buffer contents should now be: |················|
  EXPECT_TRUE(buffer.GetReadableRegion().empty());
}

}  // namespace
}  // namespace openscreen
