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

#ifndef ANONYMOUS_TOKENS_CPP_CLIENT_ANONYMOUS_TOKENS_REDEMPTION_CLIENT_H_
#define ANONYMOUS_TOKENS_CPP_CLIENT_ANONYMOUS_TOKENS_REDEMPTION_CLIENT_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "anonymous_tokens/proto/anonymous_tokens.pb.h"


namespace anonymous_tokens {

// This class generates AnonymousTokens Redemption request using the anonymous
// tokens, their respective plaintext messages and (optional) public metadata.
//
// A new instance of the AnonymousTokensRedemptionClient is needed for each
// redemption request created.
//
// This class is not thread-safe.
class AnonymousTokensRedemptionClient {
 public:
  AnonymousTokensRedemptionClient(const AnonymousTokensRedemptionClient&) =
      delete;
  AnonymousTokensRedemptionClient& operator=(
      const AnonymousTokensRedemptionClient&) = delete;

  // Creates AnonymousTokensRedemptionClient for a valid use case and key
  // version.
  static absl::StatusOr<std::unique_ptr<AnonymousTokensRedemptionClient>>
  Create(AnonymousTokensUseCase use_case, int64_t key_version);

  // Creates a redemption request for anonymous tokens against plaintext
  // messages and public metadatas (if they are set).
  absl::StatusOr<AnonymousTokensRedemptionRequest>
  CreateAnonymousTokensRedemptionRequest(
      const std::vector<RSABlindSignatureTokenWithInput>& tokens_with_inputs);

  // This method is used to process AnonymousTokensRedemptionResponse and
  // outputs a comprehensive redemption result.
  absl::StatusOr<std::vector<RSABlindSignatureRedemptionResult>>
  ProcessAnonymousTokensRedemptionResponse(
      const AnonymousTokensRedemptionResponse& redemption_response);

 private:
  // Saves plaintext message, public metadata along with the mask to use for
  // validity checks on the server response as well as correct final processing
  // of the redemption result.
  struct RedemptionInfo {
    PlaintextMessageWithPublicMetadata input;
    std::string mask;
  };

  // Takes in AnonymousTokensUseCase and a key version where the former must not
  // be undefined and the latter must be greater than 0.
  //
  // This constructor is only called from
  // AnonymousTokensRedemptionClient::Create method.
  AnonymousTokensRedemptionClient(AnonymousTokensUseCase use_case,
                                  int64_t key_version);

  const std::string use_case_;
  const int64_t key_version_;
  absl::flat_hash_map<std::string, RedemptionInfo> token_to_input_map_;
};

}  // namespace anonymous_tokens


#endif  // ANONYMOUS_TOKENS_CPP_CLIENT_ANONYMOUS_TOKENS_REDEMPTION_CLIENT_H_
