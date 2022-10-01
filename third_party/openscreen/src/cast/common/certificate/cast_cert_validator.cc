// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/cast_cert_validator.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "cast/common/certificate/cast_crl.h"
#include "cast/common/public/parsed_certificate.h"
#include "cast/common/public/trust_store.h"
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

CastDeviceCertPolicy GetAudioPolicy(
    const std::vector<const ParsedCertificate*>& path) {
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
  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i]->HasPolicyOid(audio_only_policy_oid)) {
      policy = CastDeviceCertPolicy::kAudioOnly;
      break;
    }
  }
  return policy;
}

}  // namespace

Error VerifyDeviceCert(const std::vector<std::string>& der_certs,
                       const DateTime& time,
                       std::unique_ptr<ParsedCertificate>* target_cert,
                       CastDeviceCertPolicy* policy,
                       const CastCRL* crl,
                       CRLPolicy crl_policy,
                       TrustStore* trust_store) {
  // Fail early if CRL is required but not provided.
  if (!crl && crl_policy == CRLPolicy::kCrlRequired) {
    return Error::Code::kErrCrlInvalid;
  }

  ErrorOr<TrustStore::CertificatePathResult> maybe_result_path =
      trust_store->FindCertificatePath(der_certs, time);
  if (!maybe_result_path) {
    return maybe_result_path.error();
  }
  TrustStore::CertificatePathResult& result_path = maybe_result_path.value();
  std::vector<const ParsedCertificate*> raw_path;
  raw_path.reserve(result_path.size());
  for (auto& cert : result_path) {
    raw_path.push_back(cert.get());
  }

  if (crl_policy == CRLPolicy::kCrlRequired &&
      !crl->CheckRevocation(raw_path, time)) {
    return Error::Code::kErrCertsRevoked;
  }

  *policy = GetAudioPolicy(raw_path);
  *target_cert = std::move(result_path[0]);

  return Error::Code::kNone;
}

}  // namespace cast
}  // namespace openscreen
