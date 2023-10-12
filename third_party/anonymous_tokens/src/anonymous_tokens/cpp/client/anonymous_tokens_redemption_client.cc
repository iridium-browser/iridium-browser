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

#include "anonymous_tokens/cpp/client/anonymous_tokens_redemption_client.h"

#include <memory>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "anonymous_tokens/cpp/crypto/constants.h"
#include "anonymous_tokens/proto/anonymous_tokens.pb.h"


namespace anonymous_tokens {

AnonymousTokensRedemptionClient::AnonymousTokensRedemptionClient(
    const AnonymousTokensUseCase use_case, const int64_t key_version)
    : use_case_(AnonymousTokensUseCase_Name(use_case)),
      key_version_(key_version) {}

absl::StatusOr<std::unique_ptr<AnonymousTokensRedemptionClient>>
AnonymousTokensRedemptionClient::Create(const AnonymousTokensUseCase use_case,
                                        const int64_t key_version) {
  if (key_version <= 0) {
    return absl::InvalidArgumentError("Key version must be greater than 0.");
  } else if (use_case == ANONYMOUS_TOKENS_USE_CASE_UNDEFINED) {
    return absl::InvalidArgumentError("Use case must be defined.");
  }
  return absl::WrapUnique(
      new AnonymousTokensRedemptionClient(use_case, key_version));
}

absl::StatusOr<AnonymousTokensRedemptionRequest>
AnonymousTokensRedemptionClient::CreateAnonymousTokensRedemptionRequest(
    const std::vector<RSABlindSignatureTokenWithInput>& tokens_with_inputs) {
  if (tokens_with_inputs.empty()) {
    return absl::InvalidArgumentError("Cannot create an empty request.");
  } else if (!token_to_input_map_.empty()) {
    return absl::FailedPreconditionError("Redemption request already created.");
  }
  // Request to output
  AnonymousTokensRedemptionRequest request;
  for (const RSABlindSignatureTokenWithInput& token_with_input :
       tokens_with_inputs) {
    if (token_with_input.token().token().empty()) {
      return absl::InvalidArgumentError(
          "Cannot send an empty token to redeem.");
    } else if (token_with_input.token().message_mask().empty()) {
      return absl::InvalidArgumentError("Token must have a message mask.");
    } else if (token_with_input.token().message_mask().size() <
               kRsaMessageMaskSizeInBytes32) {
      return absl::InvalidArgumentError(
          "Token must have a message mask of at least 32 bytes.");
    }
    // Check if token is repeated in the input and keep state for response
    // processing if it was not repeated.
    auto maybe_inserted = token_to_input_map_.insert(
        {token_with_input.token().token(),
         {
             .input = token_with_input.input(),
             .mask = token_with_input.token().message_mask(),
         }});
    if (!maybe_inserted.second) {
      return absl::InvalidArgumentError(
          "Token should not be repeated in the input to "
          "CreateAnonymousTokensRedemptionRequest.");
    }

    // Create the AnonymousTokenToRedeem to put in the request.
    AnonymousTokensRedemptionRequest_AnonymousTokenToRedeem* at_to_redeem =
        request.add_anonymous_tokens_to_redeem();
    at_to_redeem->set_use_case(use_case_);
    at_to_redeem->set_key_version(key_version_);
    at_to_redeem->set_public_metadata(
        token_with_input.input().public_metadata());
    at_to_redeem->set_serialized_unblinded_token(
        token_with_input.token().token());
    at_to_redeem->set_plaintext_message(
        token_with_input.input().plaintext_message());
    at_to_redeem->set_message_mask(token_with_input.token().message_mask());
  }
  return request;
}

absl::StatusOr<std::vector<RSABlindSignatureRedemptionResult>>
AnonymousTokensRedemptionClient::ProcessAnonymousTokensRedemptionResponse(
    const AnonymousTokensRedemptionResponse& redemption_response) {
  if (token_to_input_map_.empty()) {
    return absl::FailedPreconditionError(
        "A valid Redemption request was not created before calling "
        "ProcessAnonymousTokensRedemptionResponse.");
  } else if (redemption_response.anonymous_token_redemption_results().empty()) {
    return absl::InvalidArgumentError("Cannot process an empty response.");
  } else if (redemption_response.anonymous_token_redemption_results().size() !=
             token_to_input_map_.size()) {
    return absl::InvalidArgumentError(
        "Response is missing some requested token redemptions.");
  }
  // Vector to accumulate redemption results to output.
  std::vector<RSABlindSignatureRedemptionResult>
      rsa_blind_sig_redemption_results;
  // Temporary set structure to check for duplicate token in the redemption
  // response.
  absl::flat_hash_set<absl::string_view> tokens;

  // Loop over all the results in the response.
  for (const AnonymousTokensRedemptionResponse_AnonymousTokenRedemptionResult&
           redemption_result :
       redemption_response.anonymous_token_redemption_results()) {
    // Basic validity checks on the response.
    if (redemption_result.use_case() != use_case_) {
      return absl::InvalidArgumentError(
          "Use case does not match the requested use case.");
    } else if (redemption_result.key_version() != key_version_) {
      return absl::InvalidArgumentError(
          "Key version does not match the requested key version.");
    } else if (redemption_result.serialized_unblinded_token().empty()) {
      return absl::InvalidArgumentError("Token cannot be empty in response.");
    } else if (redemption_result.message_mask().empty()) {
      return absl::InvalidArgumentError("Message mask cannot be empty.");
    } else if (redemption_result.message_mask().size() <
               kRsaMessageMaskSizeInBytes32) {
      return absl::InvalidArgumentError(
          "Message mask of at least 32 bytes is required.");
    }
    // Check for duplicate in responses.
    auto maybe_inserted =
        tokens.insert(redemption_result.serialized_unblinded_token());
    if (!maybe_inserted.second) {
      return absl::InvalidArgumentError("Token was repeated in the response.");
    }

    // Retrieve redemption info associated with this redemption result.
    auto it = token_to_input_map_.find(
        redemption_result.serialized_unblinded_token());
    if (it == token_to_input_map_.end()) {
      return absl::InvalidArgumentError(
          "Server responded with some tokens whose redemptions were not "
          "requested.");
    }
    const RedemptionInfo& redemption_info = it->second;

    // Check if inputs in the redemption request and response match
    if (redemption_info.input.public_metadata() !=
        redemption_result.public_metadata()) {
      return absl::InvalidArgumentError(
          "Response metadata does not match input metadata.");
    } else if (redemption_info.input.plaintext_message() !=
               redemption_result.plaintext_message()) {
      return absl::InvalidArgumentError(
          "Response plaintext message does not match input plaintext message.");
    } else if (redemption_info.mask != redemption_result.message_mask()) {
      return absl::InvalidArgumentError(
          "Response message mask does not match input message mask.");
    }

    PlaintextMessageWithPublicMetadata message_and_metadata;
    // Put the correct plaintext message in final redemption result
    message_and_metadata.set_plaintext_message(
        redemption_result.plaintext_message());
    // Put the correct public metadata in final redemption result
    message_and_metadata.set_public_metadata(
        redemption_result.public_metadata());

    RSABlindSignatureToken token;
    // Put the correct anonymous token in final redemption result
    token.set_token(redemption_result.serialized_unblinded_token());
    // Put the correct message mask in final redemption result
    token.set_message_mask(redemption_result.message_mask());

    // Construct the final redemption result.
    RSABlindSignatureRedemptionResult final_redemption_result;
    *final_redemption_result.mutable_token_with_input()->mutable_token() =
        token;
    *final_redemption_result.mutable_token_with_input()->mutable_input() =
        message_and_metadata;
    final_redemption_result.set_redeemed(redemption_result.verified());
    final_redemption_result.set_double_spent(redemption_result.double_spent());
    // Add the redemption result to the output vector.
    rsa_blind_sig_redemption_results.push_back(
        std::move(final_redemption_result));
  }

  return rsa_blind_sig_redemption_results;
}

}  // namespace anonymous_tokens

