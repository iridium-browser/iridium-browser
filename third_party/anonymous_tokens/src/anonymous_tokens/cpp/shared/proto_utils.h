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

#ifndef ANONYMOUS_TOKENS_CPP_SHARED_PROTO_UTILS_H_
#define ANONYMOUS_TOKENS_CPP_SHARED_PROTO_UTILS_H_


#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "anonymous_tokens/proto/anonymous_tokens.pb.h"



namespace anonymous_tokens {

// Returns AnonymousTokensUseCase parsed from a string_view.
absl::StatusOr<AnonymousTokensUseCase>  ParseUseCase(
    absl::string_view use_case);

// Takes in Timestamp and converts it to absl::Time.
//
// Timestamp is defined here:
// https://developers.google.com/protocol-buffers/docs/reference/google.protobuf#timestamp
absl::StatusOr<absl::Time>  TimeFromProto(
    const Timestamp& proto);

// Takes in absl::Time and converts it to Timestamp.
//
// Timestamp is defined here:
// https://developers.google.com/protocol-buffers/docs/reference/google.protobuf#timestamp
absl::StatusOr<Timestamp>  TimeToProto(
    absl::Time time);

}  // namespace anonymous_tokens


#endif  // ANONYMOUS_TOKENS_CPP_SHARED_PROTO_UTILS_H_
