// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/channel/cast_auth_util.h"

#include <openssl/rand.h>

#include <algorithm>
#include <memory>

#include "absl/strings/str_cat.h"
#include "cast/common/certificate/cast_cert_validator.h"
#include "cast/common/certificate/cast_crl.h"
#include "cast/common/certificate/date_time.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/public/parsed_certificate.h"
#include "cast/common/public/trust_store.h"
#include "platform/api/time.h"
#include "platform/base/error.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

using ::cast::channel::AuthResponse;
using ::cast::channel::CastMessage;
using ::cast::channel::DeviceAuthMessage;
using ::cast::channel::HashAlgorithm;

namespace {

#define PARSE_ERROR_PREFIX "Failed to parse auth message: "

// The maximum number of days a cert can live for.
constexpr int kMaxSelfSignedCertLifetimeInDays = 4;

// The size of the nonce challenge in bytes.
constexpr int kNonceSizeInBytes = 16;

// The number of hours after which a nonce is regenerated.
constexpr int kNonceExpirationTimeInHours = 24;

// Extracts an embedded DeviceAuthMessage payload from an auth challenge reply
// message.
Error ParseAuthMessage(const CastMessage& challenge_reply,
                       DeviceAuthMessage* auth_message) {
  if (challenge_reply.payload_type() !=
      ::cast::channel::CastMessage_PayloadType_BINARY) {
    return Error(Error::Code::kCastV2WrongPayloadType,
                 PARSE_ERROR_PREFIX "Wrong payload type in challenge reply");
  }
  if (!challenge_reply.has_payload_binary()) {
    return Error(Error::Code::kCastV2NoPayload, PARSE_ERROR_PREFIX
                 "Payload type is binary but payload_binary field not set");
  }
  if (!auth_message->ParseFromString(challenge_reply.payload_binary())) {
    return Error(Error::Code::kCastV2PayloadParsingFailed, PARSE_ERROR_PREFIX
                 "Cannot parse binary payload into DeviceAuthMessage");
  }

  if (auth_message->has_error()) {
    std::stringstream ss;
    ss << PARSE_ERROR_PREFIX "Auth message error: "
       << auth_message->error().error_type();
    return Error(Error::Code::kCastV2MessageError, ss.str());
  }
  if (!auth_message->has_response()) {
    return Error(Error::Code::kCastV2NoResponse,
                 PARSE_ERROR_PREFIX "Auth message has no response field");
  }
  return Error::None();
}

class CastNonce {
 public:
  static CastNonce* GetInstance() {
    static CastNonce* cast_nonce = new CastNonce();
    return cast_nonce;
  }

  static const std::string& Get() {
    GetInstance()->EnsureNonceTimely();
    return GetInstance()->nonce_;
  }

 private:
  CastNonce() : nonce_(kNonceSizeInBytes, 0) { GenerateNonce(); }
  void GenerateNonce() {
    OSP_CHECK_EQ(
        RAND_bytes(reinterpret_cast<uint8_t*>(&nonce_[0]), kNonceSizeInBytes),
        1);
    nonce_generation_time_ = GetWallTimeSinceUnixEpoch();
  }

  void EnsureNonceTimely() {
    if (GetWallTimeSinceUnixEpoch() >
        (nonce_generation_time_ +
         std::chrono::hours(kNonceExpirationTimeInHours))) {
      GenerateNonce();
    }
  }

  // The nonce challenge to send to the Cast receiver.
  // The nonce is updated daily.
  std::string nonce_;
  std::chrono::seconds nonce_generation_time_;
};

// Maps an error from certificate verification to an error reported to the
// library client.  If crl_required is set to false, all revocation related
// errors are ignored.
//
// TODO(https://issuetracker.google.com/issues/193164666): It would be simpler
// to just pass the underlying verification error directly to the client.
Error MapToOpenscreenError(Error verify_error, bool crl_required) {
  switch (verify_error.code()) {
    case Error::Code::kErrCertsMissing:
      return Error(Error::Code::kCastV2PeerCertEmpty,
                   absl::StrCat("Failed to locate certificates: ",
                                verify_error.message()));
    case Error::Code::kErrCertsParse:
      return Error(Error::Code::kErrCertsParse,
                   absl::StrCat("Failed to parse certificates: ",
                                verify_error.message()));
    case Error::Code::kErrCertsDateInvalid:
      return Error(
          Error::Code::kCastV2CertNotSignedByTrustedCa,
          absl::StrCat("Failed date validity check: ", verify_error.message()));
    case Error::Code::kErrCertsVerifyGeneric:
      return Error(
          Error::Code::kCastV2CertNotSignedByTrustedCa,
          absl::StrCat("Failed with a generic certificate verification error: ",
                       verify_error.message()));
    case Error::Code::kErrCertsRestrictions:
      return Error(Error::Code::kCastV2CertNotSignedByTrustedCa,
                   absl::StrCat("Failed certificate restrictions: ",
                                verify_error.message()));
    case Error::Code::kErrCertsVerifyUntrustedCert:
      return Error(Error::Code::kCastV2CertNotSignedByTrustedCa,
                   absl::StrCat("Failed with untrusted certificate: ",
                                verify_error.message()));
    case Error::Code::kErrCrlInvalid:
      // This error is only encountered if |crl_required| is true.
      OSP_DCHECK(crl_required);
      return Error(Error::Code::kErrCrlInvalid,
                   absl::StrCat("Failed to provide a valid CRL: ",
                                verify_error.message()));
    case Error::Code::kErrCertsRevoked:
      return Error(Error::Code::kErrCertsRevoked,
                   absl::StrCat("Failed certificate revocation check: ",
                                verify_error.message()));
    case Error::Code::kNone:
      return Error::None();
    default:
      return Error(Error::Code::kCastV2CertNotSignedByTrustedCa,
                   absl::StrCat("Failed verifying cast device certificate: ",
                                verify_error.message()));
  }
}

Error VerifyAndMapDigestAlgorithm(HashAlgorithm response_digest_algorithm,
                                  DigestAlgorithm* digest_algorithm,
                                  bool enforce_sha256_checking) {
  switch (response_digest_algorithm) {
    case ::cast::channel::SHA1:
      if (enforce_sha256_checking) {
        return Error(Error::Code::kCastV2DigestUnsupported,
                     "Unsupported digest algorithm.");
      }
      *digest_algorithm = DigestAlgorithm::kSha1;
      break;
    case ::cast::channel::SHA256:
      *digest_algorithm = DigestAlgorithm::kSha256;
      break;
    default:
      return Error::Code::kCastV2DigestUnsupported;
  }
  return Error::None();
}

}  // namespace

// static
AuthContext AuthContext::Create() {
  return AuthContext(CastNonce::Get());
}

// static
AuthContext AuthContext::CreateForTest(const std::string& nonce_data) {
  std::string nonce;
  if (nonce_data.empty()) {
    nonce = std::string(kNonceSizeInBytes, '0');
  } else {
    while (nonce.size() < kNonceSizeInBytes) {
      nonce += nonce_data;
    }
    nonce.erase(kNonceSizeInBytes);
  }
  OSP_DCHECK_EQ(nonce.size(), kNonceSizeInBytes);
  return AuthContext(nonce);
}

AuthContext::AuthContext(const std::string& nonce) : nonce_(nonce) {}

AuthContext::~AuthContext() {}

Error AuthContext::VerifySenderNonce(const std::string& nonce_response,
                                     bool enforce_nonce_checking) const {
  if (nonce_ != nonce_response) {
    if (enforce_nonce_checking) {
      return Error(Error::Code::kCastV2SenderNonceMismatch,
                   "Sender nonce mismatched.");
    }
  }
  return Error::None();
}

Error VerifyTLSCertificateValidity(const ParsedCertificate& peer_cert,
                                   std::chrono::seconds verification_time) {
  // Ensure the peer cert is valid and doesn't have an excessive remaining
  // lifetime. Although it is not verified as an X.509 certificate, the entire
  // structure is signed by the AuthResponse, so the validity field from X.509
  // is repurposed as this signature's expiration.
  ErrorOr<DateTime> maybe_not_before = peer_cert.GetNotBeforeTime();
  if (!maybe_not_before) {
    return maybe_not_before.error();
  }
  ErrorOr<DateTime> maybe_not_after = peer_cert.GetNotAfterTime();
  if (!maybe_not_after) {
    return maybe_not_after.error();
  }
  std::chrono::seconds not_before_seconds =
      DateTimeToSeconds(maybe_not_before.value());
  std::chrono::seconds not_after_seconds =
      DateTimeToSeconds(maybe_not_after.value());

  std::chrono::seconds lifetime_limit =
      verification_time +
      std::chrono::hours(24 * kMaxSelfSignedCertLifetimeInDays);
  if (verification_time < not_before_seconds) {
    return Error(Error::Code::kCastV2TlsCertValidStartDateInFuture,
                 PARSE_ERROR_PREFIX
                 "Certificate's valid start date is in the future.");
  }
  if (not_after_seconds < verification_time) {
    return Error(Error::Code::kCastV2TlsCertExpired,
                 PARSE_ERROR_PREFIX "Certificate has expired.");
  }
  if (lifetime_limit < not_after_seconds) {
    return Error(Error::Code::kCastV2TlsCertValidityPeriodTooLong,
                 PARSE_ERROR_PREFIX "Peer cert lifetime is too long.");
  }
  return Error::None();
}

ErrorOr<CastDeviceCertPolicy> VerifyCredentialsImpl(
    const AuthResponse& response,
    const std::vector<uint8_t>& signature_input,
    const CRLPolicy& crl_policy,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    const DateTime& verification_time,
    bool enforce_sha256_checking);

ErrorOr<CastDeviceCertPolicy> AuthenticateChallengeReplyImpl(
    const CastMessage& challenge_reply,
    const ParsedCertificate& peer_cert,
    const AuthContext& auth_context,
    const CRLPolicy& crl_policy,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    const DateTime& verification_time) {
  DeviceAuthMessage auth_message;
  Error result = ParseAuthMessage(challenge_reply, &auth_message);
  if (!result.ok()) {
    return result;
  }

  result = VerifyTLSCertificateValidity(peer_cert,
                                        DateTimeToSeconds(verification_time));
  if (!result.ok()) {
    return result;
  }

  const AuthResponse& response = auth_message.response();
  const std::string& nonce_response = response.sender_nonce();

  result = auth_context.VerifySenderNonce(nonce_response);
  if (!result.ok()) {
    return result;
  }

  size_t nonce_response_size = nonce_response.size();
  ErrorOr<std::vector<uint8_t>> nonce_plus_peer_cert_der =
      peer_cert.SerializeToDER(nonce_response_size);
  if (!nonce_plus_peer_cert_der) {
    return nonce_plus_peer_cert_der.error();
  }
  std::copy(nonce_response.begin(), nonce_response.end(),
            &nonce_plus_peer_cert_der.value()[0]);

  return VerifyCredentialsImpl(response, nonce_plus_peer_cert_der.value(),
                               crl_policy, cast_trust_store, crl_trust_store,
                               verification_time, false);
}

ErrorOr<CastDeviceCertPolicy> AuthenticateChallengeReply(
    const CastMessage& challenge_reply,
    const ParsedCertificate& peer_cert,
    const AuthContext& auth_context,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store) {
  DateTime now = {};
  OSP_CHECK(DateTimeFromSeconds(GetWallTimeSinceUnixEpoch().count(), &now));
  CRLPolicy policy = CRLPolicy::kCrlOptional;
  return AuthenticateChallengeReplyImpl(challenge_reply, peer_cert,
                                        auth_context, policy, cast_trust_store,
                                        crl_trust_store, now);
}

ErrorOr<CastDeviceCertPolicy> AuthenticateChallengeReplyForTest(
    const CastMessage& challenge_reply,
    const ParsedCertificate& peer_cert,
    const AuthContext& auth_context,
    CRLPolicy crl_policy,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    const DateTime& verification_time) {
  return AuthenticateChallengeReplyImpl(
      challenge_reply, peer_cert, auth_context, crl_policy, cast_trust_store,
      crl_trust_store, verification_time);
}

// This function does the following
//
// * Verifies that the certificate chain |response.client_auth_certificate| +
//   |response.intermediate_certificate| is valid and chains to a trusted Cast
//   root. The list of trusted Cast roots can be overrided by providing a
//   non-nullptr |cast_trust_store|. The certificate is verified at
//   |verification_time|.
//
// * Verifies that none of the certificates in the chain are revoked based on
//   the CRL provided in the response |response.crl|. The CRL is verified to be
//   valid and its issuer certificate chains to a trusted Cast CRL root. The
//   list of trusted Cast CRL roots can be overrided by providing a non-nullptr
//   |crl_trust_store|. If |crl_policy| is kCrlOptional then the result of
//   revocation checking is ignored. The CRL is verified at |verification_time|.
//
// * Verifies that |response.signature| matches the signature of
//   |signature_input| by |response.client_auth_certificate|'s public key.
ErrorOr<CastDeviceCertPolicy> VerifyCredentialsImpl(
    const AuthResponse& response,
    const std::vector<uint8_t>& signature_input,
    const CRLPolicy& crl_policy,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    const DateTime& verification_time,
    bool enforce_sha256_checking) {
  if (response.signature().empty() && !signature_input.empty()) {
    return Error(Error::Code::kCastV2SignatureEmpty, "Signature is empty.");
  }

  // Build a single vector containing the certificate chain.
  std::vector<std::string> cert_chain;
  cert_chain.push_back(response.client_auth_certificate());
  cert_chain.insert(cert_chain.end(),
                    response.intermediate_certificate().begin(),
                    response.intermediate_certificate().end());

  // Parse the CRL.
  std::unique_ptr<CastCRL> crl;
  if (!response.crl().empty()) {
    crl = ParseAndVerifyCRL(response.crl(), verification_time, crl_trust_store);
  }

  // Perform certificate verification.
  CastDeviceCertPolicy device_policy;
  std::unique_ptr<ParsedCertificate> target_cert;
  Error verify_result =
      VerifyDeviceCert(cert_chain, verification_time, &target_cert,
                       &device_policy, crl.get(), crl_policy, cast_trust_store);

  // Handle and report errors.
  Error result = MapToOpenscreenError(verify_result,
                                      crl_policy == CRLPolicy::kCrlRequired);
  if (!result.ok()) {
    return result;
  }

  // The certificate is verified at this point.
  DigestAlgorithm digest_algorithm;
  Error digest_result = VerifyAndMapDigestAlgorithm(
      response.hash_algorithm(), &digest_algorithm, enforce_sha256_checking);
  if (!digest_result.ok()) {
    return digest_result;
  }

  ConstDataSpan signature = {
      reinterpret_cast<const uint8_t*>(response.signature().data()),
      static_cast<uint32_t>(response.signature().size())};
  ConstDataSpan siginput = {signature_input.data(),
                            static_cast<uint32_t>(signature_input.size())};
  if (!target_cert->VerifySignedData(digest_algorithm, siginput, signature)) {
    return Error(Error::Code::kCastV2SignedBlobsMismatch,
                 "Failed verifying signature over data.");
  }

  return device_policy;
}

ErrorOr<CastDeviceCertPolicy> VerifyCredentials(
    const AuthResponse& response,
    const std::vector<uint8_t>& signature_input,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    bool enforce_revocation_checking,
    bool enforce_sha256_checking) {
  DateTime now = {};
  OSP_CHECK(DateTimeFromSeconds(GetWallTimeSinceUnixEpoch().count(), &now));
  CRLPolicy policy = (enforce_revocation_checking) ? CRLPolicy::kCrlRequired
                                                   : CRLPolicy::kCrlOptional;
  return VerifyCredentialsImpl(response, signature_input, policy,
                               cast_trust_store, crl_trust_store, now,
                               enforce_sha256_checking);
}

ErrorOr<CastDeviceCertPolicy> VerifyCredentialsForTest(
    const AuthResponse& response,
    const std::vector<uint8_t>& signature_input,
    CRLPolicy crl_policy,
    TrustStore* cast_trust_store,
    TrustStore* crl_trust_store,
    const DateTime& verification_time,
    bool enforce_sha256_checking) {
  return VerifyCredentialsImpl(response, signature_input, crl_policy,
                               cast_trust_store, crl_trust_store,
                               verification_time, enforce_sha256_checking);
}

}  // namespace cast
}  // namespace openscreen
