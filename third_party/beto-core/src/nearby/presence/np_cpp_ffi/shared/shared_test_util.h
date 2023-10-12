// Copyright 2023 Google LLC
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

#ifndef NEARBY_PRESENCE_NP_CPP_FFI_TESTS_SHARED_TEST_UTILS_H_
#define NEARBY_PRESENCE_NP_CPP_FFI_TESTS_SHARED_TEST_UTILS_H_

#include "nearby_protocol.h"

inline nearby_protocol::RawAdvertisementPayload
    V0AdvEmpty(nearby_protocol::ByteBuffer<255>({1, {0x00}}));

inline nearby_protocol::RawAdvertisementPayload
    V0AdvSimple(nearby_protocol::ByteBuffer<255>({
        4,
        {0x00,       // Adv Header
         0x03,       // Public DE header
         0x15, 0x03} // Length 1 Tx Power DE with value 3
    }));

inline nearby_protocol::RawAdvertisementPayload
    V1AdvSimple(nearby_protocol::ByteBuffer<255>(
        {5,
         {
             0x20,      // V1 Advertisement header
             0x03,      // Section Header
             0x03,      // Public Identity DE header
             0x15, 0x03 // Length 1 Tx Power DE with value 3
         }}));

inline nearby_protocol::RawAdvertisementPayload
    V1AdvMultipleSections(nearby_protocol::ByteBuffer<255>(
        {10,
         {
             0x20,             // V1 Advertisement header
             0x04,             // Section Header
             0x03,             // Public Identity DE header
             0x26, 0x00, 0x46, // Length 2 Actions DE
             0x03,             // Section Header
             0x03,             // Public Identity DE header
             0x15, 0x03        // Length 1 Tx Power DE with value 3
         }}));

void test_panic_handler(nearby_protocol::PanicReason reason);

std::string generate_hex_string(size_t length);

#endif // NEARBY_PRESENCE_NP_CPP_FFI_TESTS_SHARED_TEST_UTILS_H_
