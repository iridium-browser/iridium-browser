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

#include "nearby_protocol.h"

std::string PanicReasonToString(nearby_protocol::PanicReason reason) {
  switch (reason) {
  case nearby_protocol::PanicReason::EnumCastFailed: {
    return "EnumCastFailed";
  }
  case nearby_protocol::PanicReason::AssertFailed: {
    return "AssertFailed";
  }
  case np_ffi::internal::PanicReason::InvalidActionBits: {
    return "InvalidActionBits";
  }
  }
}

void test_panic_handler(nearby_protocol::PanicReason reason) {
  std::cout << "Panicking! Reason: " << PanicReasonToString(reason);
  std::abort();
}

std::string generate_hex_string(const size_t length) {
  char *str = new char[length];

  // hexadecimal characters
  char hex_characters[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  size_t i;
  for (i = 0; i < length; i++) {
    str[i] = hex_characters[rand() % 16]; // NOLINT(cert-msc50-cpp)
  }
  str[length] = 0;
  std::string result(str, length);
  delete[] str;
  return result;
}