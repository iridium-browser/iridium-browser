// Copyright 2022 Google LLC
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

#ifndef THIRD_PARTY_NEARBY_FASTPAIR_CRYPTO_FAST_PAIR_MESSAGE_TYPE_H_
#define THIRD_PARTY_NEARBY_FASTPAIR_CRYPTO_FAST_PAIR_MESSAGE_TYPE_H_

#include <ostream>

namespace nearby {
namespace fastpair {

// Type values for Fast Pair messages.
enum class FastPairMessageType {
  // Unknown message type.
  kUnknown = 0,
  // Key-based Pairing Request.
  kKeyBasedPairingRequest = 1,
  // Key-based Pairing Response.
  kKeyBasedPairingResponse = 2,
  // Seeker's passkey.
  kSeekersPasskey = 3,
  // Provider's passkey.
  kProvidersPasskey = 4,
};

std::ostream& operator<<(std::ostream& stream,
                         FastPairMessageType message_type);

}  // namespace fastpair
}  // namespace nearby

#endif  // THIRD_PARTY_NEARBY_FASTPAIR_CRYPTO_FAST_PAIR_MESSAGE_TYPE_H_
