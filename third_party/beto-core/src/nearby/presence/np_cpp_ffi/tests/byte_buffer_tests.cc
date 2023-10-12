// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on a "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "nearby_protocol.h"
#include "shared_test_util.h"

#include "absl/strings/escaping.h"
#include "gtest/gtest.h"

TEST(ByteBufferTests, ByteBufferMaxLength) {
  // Each hex byte takes up 2 characters so length 510 string = 255 bytes of hex
  auto str_bytes = generate_hex_string(510);
  auto bytes = absl::HexStringToBytes(str_bytes);
  auto buffer = nearby_protocol::ByteBuffer<255>::CopyFrom(bytes);
  ASSERT_TRUE(buffer.ok());
  auto string = buffer.value().ToString();
  ASSERT_EQ(bytes, string);
}

TEST(ByteBufferTooLarge, ByteBufferInvalidLength) {
  // 256 bytes should fail
  auto str_bytes = generate_hex_string(512);
  auto bytes = absl::HexStringToBytes(str_bytes);
  auto buffer = nearby_protocol::ByteBuffer<255>::CopyFrom(bytes);
  ASSERT_FALSE(buffer.ok());
}

TEST(ByteBufferTests, ByteBufferRoundTrip) {
  auto bytes = absl::HexStringToBytes("2003031503");
  auto buffer = nearby_protocol::ByteBuffer<255>::CopyFrom(bytes);
  auto string = buffer.value().ToString();
  ASSERT_EQ(bytes, string);
}

TEST(ByteBufferTests, ByteBufferPayloadWrongSize) {
  auto bytes = absl::HexStringToBytes("1111111111111111111111");
  auto buffer = nearby_protocol::ByteBuffer<10>::CopyFrom(bytes);
  ASSERT_FALSE(buffer.ok());
}

TEST(ByteBufferTests, ByteBufferEmptyString) {
  auto bytes = absl::HexStringToBytes("");
  auto buffer = nearby_protocol::ByteBuffer<10>::CopyFrom(bytes);
  ASSERT_TRUE(buffer.ok());
}

TEST(ByteBufferTests, ByteBufferToVector) {
  auto bytes = absl::HexStringToBytes("1234567890");
  auto buffer = nearby_protocol::ByteBuffer<100>::CopyFrom(bytes);
  auto vec = buffer.value().ToVector();
  std::vector<uint8_t> expected{0x12, 0x34, 0x56, 0x78, 0x90};
  ASSERT_EQ(vec, expected);
}

TEST(ByteBufferTests, ByteBufferEndToEndPayloadAsString) {
  std::string bytes = absl::HexStringToBytes("2003031503");
  auto buffer = nearby_protocol::ByteBuffer<255>::CopyFrom(bytes);
  ASSERT_TRUE(buffer.ok());

  nearby_protocol::RawAdvertisementPayload adv(buffer.value());

  auto credential_book = nearby_protocol::CredentialBook::TryCreate();
  auto str = nearby_protocol::Deserializer::DeserializeAdvertisement(
                 adv, credential_book.value())
                 .IntoV1()
                 .TryGetSection(0)
                 .value()
                 .TryGetDataElement(0)
                 .value()
                 .GetPayload()
                 .ToString();
  ASSERT_EQ(str, absl::HexStringToBytes("03"));
}
