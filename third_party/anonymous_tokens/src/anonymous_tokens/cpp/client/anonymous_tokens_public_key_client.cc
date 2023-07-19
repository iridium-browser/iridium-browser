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

#include "anonymous_tokens/cpp/client/anonymous_tokens_public_key_client.h"

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "anonymous_tokens/cpp/shared/proto_utils.h"
#include "anonymous_tokens/cpp/shared/status_utils.h"


namespace anonymous_tokens {

namespace {

absl::Status ValidityChecksForRequestCreation(
    const AnonymousTokensPublicKeysGetRequest& public_key_request,
    AnonymousTokensUseCase use_case, int64_t key_version,
    absl::Time key_validity_start_time,
    absl::optional<absl::Time> key_validity_end_time) {
  // Basic validity checks.
  if (!public_key_request.use_case().empty()) {
    // Cannot create a public key request more than once using the same client.
    return absl::FailedPreconditionError(
        "Public Key request is already created.");
  } else if (use_case == ANONYMOUS_TOKENS_USE_CASE_UNDEFINED) {
    // Use case must be valid.
    return absl::InvalidArgumentError("Use case must be defined.");
  } else if (key_version < 0) {
    // Key version cannot be negative.
    return absl::InvalidArgumentError(
        "Key Version in an AnonymousTokensPublicKeysGetRequest "
        "must be 0 or greater than 0.");
  } else if (key_validity_end_time.has_value() &&
             key_validity_end_time.value() <= key_validity_start_time) {
    // Key cannot expire before or at its validity start time.
    return absl::InvalidArgumentError(
        "Key validity start time can not be the same or after key validity "
        "end time (if set).");
  } else if (key_validity_end_time.has_value() &&
             key_validity_end_time.value() < absl::Now()) {
    // Key's expiry time cannot be in the past.
    return absl::InvalidArgumentError(
        "Requested Key expiry time (if set) must not be in the past.");
  }
  return absl::OkStatus();
}

absl::Status ValidityChecksForResponseProcessing(
    const RSABlindSignaturePublicKey& public_key,
    const AnonymousTokensPublicKeysGetRequest& public_key_request) {
  // Basic validity checks.
  ANON_TOKENS_RETURN_IF_ERROR(ParseUseCase(public_key.use_case()).status());
  if (public_key_request.use_case() != public_key.use_case()) {
    // Use case must be the same as the requested use case.
    return absl::InvalidArgumentError(
        "Public key is not for the Use Case requested.");
  } else if (public_key.key_version() <= 0) {
    // Key version must be greater than zero.
    return absl::InvalidArgumentError(
        "Key_version cannot be zero or negative.");
  } else if (public_key_request.key_version() > 0 &&
             public_key_request.key_version() != public_key.key_version()) {
    // Key version must be the same as the requested key version if the
    // latter was greater than zero (explicit).
    return absl::InvalidArgumentError(
        "Public key is not for the Key Version requested.");
  } else if (public_key.salt_length() <= 0) {
    // We do not want deterministic signatures and negative lengths are
    // invalid.
    return absl::InvalidArgumentError(
        "Salt length must not be zero or negative.");
  } else if (public_key.key_size() < 256) {
    // Key size must be a valid value.
    return absl::InvalidArgumentError(
        "Key_size cannot be less than 256 bytes.");
  } else if (public_key.message_mask_type() == AT_MESSAGE_MASK_TYPE_UNDEFINED) {
    return absl::InvalidArgumentError("Message mask type must be defined.");
  } else if (public_key.message_mask_size() < 32) {
    return absl::InvalidArgumentError("Message mask size must be at least 32.");
  } else if (public_key.serialized_public_key().empty()) {
    // Public key should not be empty.
    return absl::InvalidArgumentError(
        "Public Key not set for a particular use case and key version.");
  } else if (!public_key.has_key_validity_start_time()) {
    // Public key must have a key validity start time.
    return absl::InvalidArgumentError(
        "Public Key has no set validity start time.");
  }
  ANON_TOKENS_ASSIGN_OR_RETURN(
      absl::Time requested_key_validity_start_time,
      TimeFromProto(public_key_request.key_validity_start_time()));
  ANON_TOKENS_ASSIGN_OR_RETURN(
      absl::Time public_key_validity_start_time,
      TimeFromProto(public_key.key_validity_start_time()));
  if (requested_key_validity_start_time < public_key_validity_start_time) {
    // Public key start time must be at or before the requested validity
    // start time.
    return absl::InvalidArgumentError(
        "Public Key is not valid at the requested validity start time.");
  } else if (public_key_request.has_key_validity_end_time() &&
             !public_key.has_expiration_time()) {
    // If a public key with explicit expiration was requested, indefinitely
    // valid public key should not be returned.
    return absl::InvalidArgumentError("Public Key does not expire.");
  } else if (!public_key_request.has_key_validity_end_time() &&
             public_key.has_expiration_time()) {
    // If an indefinitely valid public key was requested, a public key with
    // an expiry time should not be returned.
    return absl::InvalidArgumentError("Public Key is not indefinitely valid");
  }
  absl::optional<absl::Time> public_key_expiry_time = absl::nullopt;
  if (public_key_request.has_key_validity_end_time() &&
      public_key.has_expiration_time()) {
    ANON_TOKENS_ASSIGN_OR_RETURN(
        absl::Time requested_key_expiry_time,
        TimeFromProto(public_key_request.key_validity_end_time()));
    ANON_TOKENS_ASSIGN_OR_RETURN(public_key_expiry_time,
                                 TimeFromProto(public_key.expiration_time()));
    if (requested_key_expiry_time < public_key_expiry_time) {
      // Public key expiry time must be at or before the requested expiry
      // time.
      return absl::InvalidArgumentError(
          "Public Key expires after the requested expiry time.");
    } else if (public_key_expiry_time <= public_key_validity_start_time) {
      // Key cannot expire before its validity period has even started.
      return absl::InvalidArgumentError(
          "Public Key cannot be expired at or before its validity start "
          "time.");
    } else if (public_key_expiry_time <= absl::Now()) {
      // Key cannot be already expired.
      return absl::InvalidArgumentError("Expired Public Key was returned");
    }
  }
  RSAPublicKey rsa_public_key_pb;
  if (!rsa_public_key_pb.ParseFromString(public_key.serialized_public_key())) {
    return absl::InvalidArgumentError("Public key is malformed.");
  }
  if (rsa_public_key_pb.n().size() !=
      static_cast<size_t>(public_key.key_size())) {
    return absl::InvalidArgumentError(
        "Actual and given Public Key sizes are different.");
  }
  return absl::OkStatus();
}

}  // namespace

absl::StatusOr<std::unique_ptr<AnonymousTokensPublicKeysGetClient>>
AnonymousTokensPublicKeysGetClient::Create() {
  return absl::WrapUnique(new AnonymousTokensPublicKeysGetClient());
}

absl::StatusOr<AnonymousTokensPublicKeysGetRequest>
AnonymousTokensPublicKeysGetClient::CreateAnonymousTokensPublicKeysGetRequest(
    AnonymousTokensUseCase use_case, int64_t key_version,
    absl::Time key_validity_start_time,
    absl::optional<absl::Time> key_validity_end_time) {
  ANON_TOKENS_RETURN_IF_ERROR(ValidityChecksForRequestCreation(
      public_key_request_, use_case, key_version, key_validity_start_time,
      key_validity_end_time));
  AnonymousTokensPublicKeysGetRequest request;
  request.set_use_case(AnonymousTokensUseCase_Name(use_case));
  request.set_key_version(key_version);
  ANON_TOKENS_ASSIGN_OR_RETURN(*(request.mutable_key_validity_start_time()),
                               TimeToProto(key_validity_start_time));
  if (key_validity_end_time.has_value()) {
    ANON_TOKENS_ASSIGN_OR_RETURN(*(request.mutable_key_validity_end_time()),
                                 TimeToProto(*key_validity_end_time));
  }
  // Record the request.
  public_key_request_ = request;
  return request;
}

absl::StatusOr<std::vector<RSABlindSignaturePublicKey>>
AnonymousTokensPublicKeysGetClient::
    ProcessAnonymousTokensRSAPublicKeysGetResponse(
        const AnonymousTokensPublicKeysGetResponse&
            rsa_public_key_get_response) {
  if (public_key_request_.use_case().empty()) {
    // Create request method must be called before processing the response.
    return absl::FailedPreconditionError(
        "CreateAnonymousTokensPublicKeysGetRequest has not been called yet.");
  }
  std::vector<RSABlindSignaturePublicKey> rsa_public_keys;
  // Temporary set structure to identify duplicate responses.
  absl::flat_hash_set<std::tuple<std::string, int>> use_case_and_key_version;
  for (const auto& resp_public_key :
       rsa_public_key_get_response.rsa_public_keys()) {
    ANON_TOKENS_RETURN_IF_ERROR(ValidityChecksForResponseProcessing(
        resp_public_key, public_key_request_));

    // Extract use case and key version.
    ANON_TOKENS_ASSIGN_OR_RETURN(AnonymousTokensUseCase use_case,
                                 ParseUseCase(resp_public_key.use_case()));
    int key_version = resp_public_key.key_version();

    // Check for duplicate responses.
    std::tuple<std::string, int> use_case_key_version_tuple =
        std::make_pair(AnonymousTokensUseCase_Name(use_case), key_version);
    if (use_case_and_key_version.contains(use_case_key_version_tuple)) {
      return absl::InvalidArgumentError(
          "Use Case and Key Version combination must not be repeated in the "
          "response.");
    } else {
      use_case_and_key_version.insert(use_case_key_version_tuple);
    }

    rsa_public_keys.push_back(resp_public_key);
  }

  return rsa_public_keys;
}

}  // namespace anonymous_tokens

