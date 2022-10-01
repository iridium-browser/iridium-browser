// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_SENDER_CHANNEL_CAST_AUTH_UTIL_H_
#define CAST_SENDER_CHANNEL_CAST_AUTH_UTIL_H_

#include <chrono>
#include <string>
#include <vector>

#include "cast/common/certificate/cast_cert_validator.h"
#include "platform/base/error.h"

namespace cast {
namespace channel {
class AuthResponse;
class CastMessage;
}  // namespace channel
}  // namespace cast

namespace openscreen {
namespace cast {

enum class CRLPolicy;
struct DateTime;
class TrustStore;
class ParsedCertificate;

class AuthContext {
 public:
  ~AuthContext();

  // Get an auth challenge context.
  // The same context must be used in the challenge and reply.
  static AuthContext Create();

  // Create a context with some seed nonce data for testing.
  static AuthContext CreateForTest(const std::string& nonce_data);

  // Verifies the nonce received in the response is equivalent to the one sent.
  // Returns success if |nonce_response| matches nonce_
  Error VerifySenderNonce(const std::string& nonce_response,
                          bool enforce_nonce_checking = false) const;

  // The nonce challenge.
  const std::string& nonce() const { return nonce_; }

 private:
  explicit AuthContext(const std::string& nonce);

  const std::string nonce_;
};

// Authenticates the given |challenge_reply|:
// 1. Signature contained in the reply is valid.
// 2. certificate used to sign is rooted to a trusted CA.
ErrorOr<CastDeviceCertPolicy> AuthenticateChallengeReply(
    const ::cast::channel::CastMessage& challenge_reply,
    const ParsedCertificate& peer_cert,
    const AuthContext& auth_context,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store);

// Exposed for testing only.
//
// Overloaded version of AuthenticateChallengeReply that allows modifying the
// crl policy and verification times.
ErrorOr<CastDeviceCertPolicy> AuthenticateChallengeReplyForTest(
    const ::cast::channel::CastMessage& challenge_reply,
    const ParsedCertificate& peer_cert,
    const AuthContext& auth_context,
    CRLPolicy crl_policy,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    const DateTime& verification_time);

// Performs a quick check of the TLS certificate for time validity requirements.
Error VerifyTLSCertificateValidity(const ParsedCertificate& peer_cert,
                                   std::chrono::seconds verification_time);

// Auth-library specific implementation of cryptographic signature verification
// routines. Verifies that |response| contains a valid signature of
// |signature_input|.
ErrorOr<CastDeviceCertPolicy> VerifyCredentials(
    const ::cast::channel::AuthResponse& response,
    const std::vector<uint8_t>& signature_input,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    bool enforce_revocation_checking = false,
    bool enforce_sha256_checking = false);

// Exposed for testing only.
//
// Overloaded version of VerifyCredentials that allows modifying the crl policy
// and verification times.
ErrorOr<CastDeviceCertPolicy> VerifyCredentialsForTest(
    const ::cast::channel::AuthResponse& response,
    const std::vector<uint8_t>& signature_input,
    CRLPolicy crl_policy,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    const DateTime& verification_time,
    bool enforce_sha256_checking = false);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_SENDER_CHANNEL_CAST_AUTH_UTIL_H_
