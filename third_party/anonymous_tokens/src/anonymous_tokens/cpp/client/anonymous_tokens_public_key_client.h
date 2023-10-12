// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ANONYMOUS_TOKENS_CPP_CLIENT_ANONYMOUS_TOKENS_PUBLIC_KEY_CLIENT_H_
#define ANONYMOUS_TOKENS_CPP_CLIENT_ANONYMOUS_TOKENS_PUBLIC_KEY_CLIENT_H_

#include <cstddef>
#include <memory>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "anonymous_tokens/proto/anonymous_tokens.pb.h"


namespace anonymous_tokens {

// This class generates AnonymousTokens Public Key(s) Get request and processes
// the response.
//
// Each execution of the AnonymousTokens Public Key(s) Get protocol requires a
// new instance of the AnonymousTokensPublicKeyGetClient.
//
// This class is not thread-safe.
class AnonymousTokensPublicKeysGetClient {
 public:
  // AnonymousTokensPublicKeyGetClient is neither copyable nor copy assignable.
  AnonymousTokensPublicKeysGetClient(
      const AnonymousTokensPublicKeysGetClient&) = delete;
  AnonymousTokensPublicKeysGetClient& operator=(
      const AnonymousTokensPublicKeysGetClient&) = delete;

  // Creates AnonymousTokensPublicKeyGetClient.
  static absl::StatusOr<std::unique_ptr<AnonymousTokensPublicKeysGetClient>>
  Create();

  // This method is used to create requests to retrieve public key(s) from the
  // server.
  //
  // Key version defaults to 0. A value of 0 means that all key(s) for use_case
  // that adhere to the validity time window in the request, will be returned.
  //
  // key_validity_start_time defaults to absl::Now(). key_validity_start_time
  // indicates that the public key(s) expected in the response must have their
  // valid period start time before or at this time.
  //
  // key_validity_end_time defaults to null which indicates that only
  // indefinitely valid key(s) must be returned. However if, this time is set,
  // the key(s) returned must expire before or at this indicated time.
  absl::StatusOr<AnonymousTokensPublicKeysGetRequest>
  CreateAnonymousTokensPublicKeysGetRequest(
      AnonymousTokensUseCase use_case, int64_t key_version = 0,
      absl::Time key_validity_start_time = absl::Now(),
      absl::optional<absl::Time> key_validity_end_time = absl::nullopt);

  // This method is used to process the AnonymousTokensPublicKeysGetResponse
  // sent by the public key server.
  absl::StatusOr<std::vector<RSABlindSignaturePublicKey>>
  ProcessAnonymousTokensRSAPublicKeysGetResponse(
      const AnonymousTokensPublicKeysGetResponse& rsa_public_key_get_response);

 private:
  AnonymousTokensPublicKeysGetClient() = default;

  // Request created by CreateAnonymousTokensPublicKeysGetRequest is stored here
  // so that it can be used in processing of the server response.
  AnonymousTokensPublicKeysGetRequest public_key_request_;
};

}  // namespace anonymous_tokens


#endif  // ANONYMOUS_TOKENS_CPP_CLIENT_ANONYMOUS_TOKENS_PUBLIC_KEY_CLIENT_H_
