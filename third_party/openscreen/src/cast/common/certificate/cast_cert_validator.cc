// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/cast_cert_validator.h"

#include <openssl/digest.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "cast/common/certificate/cast_cert_validator_internal.h"
#include "cast/common/certificate/cast_crl.h"
#include "cast/common/certificate/cast_trust_store.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

// Returns the OID for the Audio-Only Cast policy
// (1.3.6.1.4.1.11129.2.5.2) in DER form.
const ConstDataSpan& AudioOnlyPolicyOid() {
  static const uint8_t kAudioOnlyPolicy[] = {0x2B, 0x06, 0x01, 0x04, 0x01,
                                             0xD6, 0x79, 0x02, 0x05, 0x02};
  static ConstDataSpan kPolicySpan{kAudioOnlyPolicy, sizeof(kAudioOnlyPolicy)};
  return kPolicySpan;
}

class CertVerificationContextImpl final : public CertVerificationContext {
 public:
  CertVerificationContextImpl(bssl::UniquePtr<EVP_PKEY> cert,
                              std::string common_name)
      : public_key_{std::move(cert)}, common_name_(std::move(common_name)) {}

  ~CertVerificationContextImpl() override = default;

  bool VerifySignatureOverData(
      const ConstDataSpan& signature,
      const ConstDataSpan& data,
      DigestAlgorithm digest_algorithm) const override {
    const EVP_MD* digest;
    switch (digest_algorithm) {
      case DigestAlgorithm::kSha1:
        digest = EVP_sha1();
        break;
      case DigestAlgorithm::kSha256:
        digest = EVP_sha256();
        break;
      case DigestAlgorithm::kSha384:
        digest = EVP_sha384();
        break;
      case DigestAlgorithm::kSha512:
        digest = EVP_sha512();
        break;
      default:
        return false;
    }

    return VerifySignedData(digest, public_key_.get(), data, signature);
  }

  const std::string& GetCommonName() const override { return common_name_; }

 private:
  const bssl::UniquePtr<EVP_PKEY> public_key_;
  const std::string common_name_;
};

CastDeviceCertPolicy GetAudioPolicy(const std::vector<X509*>& path) {
  // Cast device certificates use the policy 1.3.6.1.4.1.11129.2.5.2 to indicate
  // it is *restricted* to an audio-only device whereas the absence of a policy
  // means it is unrestricted.
  //
  // This is somewhat different than RFC 5280's notion of policies, so policies
  // are checked separately outside of path building.
  //
  // See the unit-tests VerifyCastDeviceCertTest.Policies* for some
  // concrete examples of how this works.
  //
  // Iterate over all the certificates, including the root certificate. If any
  // certificate contains the audio-only policy, the whole chain is considered
  // constrained to audio-only device certificates.
  //
  // Policy mappings are not accounted for. The expectation is that top-level
  // intermediates issued with audio-only will have no mappings. If subsequent
  // certificates in the chain do, it won't matter as the chain is already
  // restricted to being audio-only.
  CastDeviceCertPolicy policy = CastDeviceCertPolicy::kUnrestricted;
  const ConstDataSpan& audio_only_policy_oid = AudioOnlyPolicyOid();
  for (uint32_t i = 0;
       i < path.size() && policy != CastDeviceCertPolicy::kAudioOnly; ++i) {
    X509* cert = path[path.size() - 1 - i];

    // Gets an index into the extension list that points to the
    // certificatePolicies extension, if it exists, -1 otherwise.
    int pos = X509_get_ext_by_NID(cert, NID_certificate_policies, -1);
    if (pos != -1) {
      X509_EXTENSION* policies_extension = X509_get_ext(cert, pos);
      const ASN1_STRING* value = X509_EXTENSION_get_data(policies_extension);
      const uint8_t* in = ASN1_STRING_get0_data(value);
      CERTIFICATEPOLICIES* policies =
          d2i_CERTIFICATEPOLICIES(nullptr, &in, ASN1_STRING_length(value));

      if (policies) {
        // Check for |audio_only_policy_oid| in the set of policies.
        uint32_t policy_count = sk_POLICYINFO_num(policies);
        for (uint32_t j = 0; j < policy_count; ++j) {
          POLICYINFO* info = sk_POLICYINFO_value(policies, j);
          if (OBJ_length(info->policyid) == audio_only_policy_oid.length &&
              memcmp(OBJ_get0_data(info->policyid), audio_only_policy_oid.data,
                     audio_only_policy_oid.length) == 0) {
            policy = CastDeviceCertPolicy::kAudioOnly;
            break;
          }
        }
        CERTIFICATEPOLICIES_free(policies);
      }
    }
  }
  return policy;
}

}  // namespace

Error VerifyDeviceCert(const std::vector<std::string>& der_certs,
                       const DateTime& time,
                       std::unique_ptr<CertVerificationContext>* context,
                       CastDeviceCertPolicy* policy,
                       const CastCRL* crl,
                       CRLPolicy crl_policy,
                       TrustStore* trust_store) {
  if (!trust_store) {
    trust_store = CastTrustStore::GetInstance()->trust_store();
  }

  // Fail early if CRL is required but not provided.
  if (!crl && crl_policy == CRLPolicy::kCrlRequired) {
    return Error::Code::kErrCrlInvalid;
  }

  CertificatePathResult result_path = {};
  Error error = FindCertificatePath(der_certs, time, &result_path, trust_store);
  if (!error.ok()) {
    return error;
  }

  if (crl_policy == CRLPolicy::kCrlRequired &&
      !crl->CheckRevocation(result_path.path, time)) {
    return Error::Code::kErrCertsRevoked;
  }

  *policy = GetAudioPolicy(result_path.path);

  // Finally, make sure there is a common name to give to
  // CertVerificationContextImpl.
  X509_NAME* target_subject =
      X509_get_subject_name(result_path.target_cert.get());
  int len =
      X509_NAME_get_text_by_NID(target_subject, NID_commonName, nullptr, 0);
  if (len <= 0) {
    return Error::Code::kErrCertsRestrictions;
  }
  // X509_NAME_get_text_by_NID writes one more byte than it reports, for a
  // trailing NUL.
  std::string common_name(len + 1, 0);
  len = X509_NAME_get_text_by_NID(target_subject, NID_commonName,
                                  &common_name[0], common_name.size());
  if (len <= 0) {
    return Error::Code::kErrCertsRestrictions;
  }
  common_name.resize(len);

  context->reset(new CertVerificationContextImpl(
      bssl::UniquePtr<EVP_PKEY>{X509_get_pubkey(result_path.target_cert.get())},
      std::move(common_name)));

  return Error::Code::kNone;
}

}  // namespace cast
}  // namespace openscreen
