// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "securegcm/java_util.h"

#include <limits>

#include "gtest/gtest.h"

namespace securegcm {

using securemessage::ByteBuffer;

namespace {

int32_t kMinInt32 = std::numeric_limits<int32_t>::min();
int32_t kMaxInt32 = std::numeric_limits<int32_t>::max();

}  // namespace


TEST(JavaUtilTest, TestJavaAdd_InRange) {
  EXPECT_EQ(2, java_util::JavaAdd(1, 1));
}

TEST(JavaUtilTest, TestJavaAdd_Underflow) {
  EXPECT_EQ(kMaxInt32, java_util::JavaAdd(kMinInt32, -1));
  EXPECT_EQ(kMaxInt32 - 1, java_util::JavaAdd(kMinInt32, -2));
  EXPECT_EQ(1, java_util::JavaAdd(kMinInt32, -kMaxInt32));
}

TEST(JavaUtilTest, TestJavaAdd_Overflow) {
  EXPECT_EQ(kMinInt32, java_util::JavaAdd(kMaxInt32, 1));
  EXPECT_EQ(kMinInt32 + 1, java_util::JavaAdd(kMaxInt32, 2));
  EXPECT_EQ(-2, java_util::JavaAdd(kMaxInt32, kMaxInt32));
}

TEST(JavaUtilTest, TestJavaMultiply_InRange) {
  EXPECT_EQ(4, java_util::JavaAdd(2, 2));
}

TEST(JavaUtilTest, TestJavaMultiply_Underflow) {
  EXPECT_EQ(0, java_util::JavaMultiply(kMinInt32, 2));
  EXPECT_EQ(-(kMinInt32 / 2), java_util::JavaMultiply(kMinInt32 / 2, 3));
  EXPECT_EQ(kMinInt32, java_util::JavaMultiply(kMinInt32, kMaxInt32));
}

TEST(JavaUtilTest, TestJavaMultiply_Overflow) {
  EXPECT_EQ(-2, java_util::JavaMultiply(kMaxInt32, 2));
  EXPECT_EQ(kMaxInt32 - 2, java_util::JavaMultiply(kMaxInt32, 3));
  EXPECT_EQ(1, java_util::JavaMultiply(kMaxInt32, kMaxInt32));
}

TEST(JavaUtilTest, TestJavaHashCode_EmptyBytes) {
  EXPECT_EQ(1, java_util::JavaHashCode(ByteBuffer()));
}

TEST(JavaUtilTest, TestJavaHashCode_LongByteArray) {
  const uint8_t kBytes[] = {
      0x93, 0x75, 0xE1, 0x2E, 0x26, 0x28, 0x54, 0x8C, 0xD9, 0x5C, 0x48, 0x7A,
      0x07, 0x53, 0x4E, 0xED, 0x28, 0x52, 0x5D, 0x41, 0xE3, 0x18, 0x84, 0x84,
      0x5F, 0xF6, 0x89, 0x98, 0x25, 0x1E, 0xD9, 0x6C, 0x85, 0xF3, 0x5A, 0x83,
      0x39, 0x37, 0x4E, 0x77, 0x95, 0xB5, 0x58, 0x7C, 0xD2, 0x55, 0xA0, 0x86,
      0x13, 0x3F, 0xBF, 0x85, 0xD3, 0xE0, 0x28, 0x90, 0x17, 0x3D, 0x2E, 0xD4,
      0x4D, 0x95, 0x9C, 0xAE, 0xAD, 0x8A, 0x05, 0x91, 0x5D, 0xC6, 0x4B, 0x09,
      0xB2, 0xD9, 0x34, 0x64, 0x07, 0x7B, 0x07, 0x8C, 0xA6, 0xC7, 0x1C, 0x10,
      0x34, 0xD4, 0x30, 0x80, 0x03, 0x4F, 0x2C, 0x70};
  const int32_t kExpectedHashCode = 1983685004;
  EXPECT_EQ(kExpectedHashCode,
            java_util::JavaHashCode(ByteBuffer(kBytes, sizeof(kBytes))));
}

}  // namespace securegcm
