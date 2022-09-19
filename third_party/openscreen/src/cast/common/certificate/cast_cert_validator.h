// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_CAST_CERT_VALIDATOR_H_
#define CAST_COMMON_CERTIFICATE_CAST_CERT_VALIDATOR_H_

#include <memory>
#include <string>
#include <vector>

#include "cast/common/public/certificate_types.h"
#include "platform/base/error.h"
#include "platform/base/macros.h"

namespace openscreen {
namespace cast {

class CastCRL;
class ParsedCertificate;
class TrustStore;

// Describes the policy for a Device certificate.
enum class CastDeviceCertPolicy {
  // The device certificate is unrestricted.
  kUnrestricted,

  // The device certificate is for an audio-only device.
  kAudioOnly,
};

enum class CRLPolicy {
  // Revocation is only checked if a CRL is provided.
  kCrlOptional,

  // Revocation is always checked. A missing CRL results in failure.
  kCrlRequired,
};

// Verifies a cast device certificate given a chain of DER-encoded certificates.
//
// Inputs:
//
// * |der_certs| is a chain of DER-encoded certificates:
//   * |der_certs[0]| is the target certificate (i.e. the device certificate).
//   * |der_certs[1..n-1]| are intermediates certificates to use in path
//     building.  Their ordering does not matter.
//
// * |time| is the timestamp to use for determining if the certificate is
//   expired.
//
// * |crl| is the CRL to check for certificate revocation status.
//   If this is a nullptr, then revocation checking is currently disabled.
//
// * |crl_policy| is for choosing how to handle the absence of a CRL.
//   If CRL_REQUIRED is passed, then an empty |crl| input would result
//   in a failed verification. Otherwise, |crl| is ignored if it is absent.
//
// * |trust_store| is a set of trusted certificates that may act as root CAs
//   during chain verification.
//
// Outputs:
//
// Returns Error::Code::kNone on success.  Otherwise, the corresponding
// Error::Code.  On success, the output parameters are filled with more details:
//
//   * |target_cert| is filled with a ParsedCertificate corresponding to the
//     device's certificate and can be used to verify signatures using the
//     device certificate's public key, as well as to extract other properties
//     from the device certificate (Common Name).
//   * |policy| is filled with an indication of the device certificate's policy
//     (i.e. is it for audio-only devices or is it unrestricted?)
[[nodiscard]] Error VerifyDeviceCert(
    const std::vector<std::string>& der_certs,
    const DateTime& time,
    std::unique_ptr<ParsedCertificate>* target_cert,
    CastDeviceCertPolicy* policy,
    const CastCRL* crl,
    CRLPolicy crl_policy,
    TrustStore* trust_store);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_CAST_CERT_VALIDATOR_H_
