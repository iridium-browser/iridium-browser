/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nearby_protocol.h"

#include <cstdint>

// Fuzz input data beginning with a valid header
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Won't fit into a FfiByteBuffer<255>
  if (size > 255) {
    return -1;
  }

  nearby_protocol::FfiByteBuffer<255> raw_bytes;
  memcpy(&raw_bytes.bytes, data, size);
  raw_bytes.len = size;

  nearby_protocol::RawAdvertisementPayload payload(
      (nearby_protocol::ByteBuffer<255>(raw_bytes)));

  auto credential_book = nearby_protocol::CredentialBook::TryCreate();
  if (!credential_book.ok()) {
    printf("Error: create Credential book failed\n");
    __builtin_trap();
  }

  // Force it to go down the v0 deserialization path
  raw_bytes.bytes[0] = 0x00;

  auto v0_result = nearby_protocol::Deserializer::DeserializeAdvertisement(
      payload, credential_book.value());

  // Force it down the v1 deserialization path
  raw_bytes.bytes[0] = 0x20;

  auto v1_result = nearby_protocol::Deserializer::DeserializeAdvertisement(
      payload, credential_book.value());

  return 0;
}