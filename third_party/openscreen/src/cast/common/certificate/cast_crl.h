// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_CAST_CRL_H_
#define CAST_COMMON_CERTIFICATE_CAST_CRL_H_

#include <openssl/x509.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cast/common/certificate/cast_cert_validator.h"
#include "cast/common/certificate/proto/revocation.pb.h"
#include "platform/base/macros.h"

namespace openscreen {
namespace cast {

// TODO(crbug.com/openscreen/90): Remove these after Chromium is migrated to
// openscreen::cast
using CrlBundle = ::cast::certificate::CrlBundle;
using Crl = ::cast::certificate::Crl;
using TbsCrl = ::cast::certificate::TbsCrl;
using SerialNumberRange = ::cast::certificate::SerialNumberRange;

// This class represents the certificate revocation list information parsed from
// the binary in a protobuf message.
class CastCRL {
 public:
  CastCRL(const TbsCrl& tbs_crl, const DateTime& overall_not_after);
  ~CastCRL();

  // Verifies the revocation status of a cast device certificate given a chain
  // of X.509 certificates.
  //
  // Inputs:
  // * |trusted_chain| is the chain of verified certificates, starting with
  //   trust anchor.
  //
  // * |time| is the timestamp to use for determining if the certificate is
  //   revoked.
  //
  // Output:
  // Returns true if no certificate in the chain was revoked.
  bool CheckRevocation(const std::vector<X509*>& trusted_chain,
                       const DateTime& time) const;

 private:
  struct SerialNumberRange {
    uint64_t first_serial;
    uint64_t last_serial;
  };

  DateTime not_before_;
  DateTime not_after_;

  // Revoked public key hashes.
  // The values consist of the SHA256 hash of the SubjectPublicKeyInfo.
  std::unordered_set<std::string> revoked_hashes_;

  // Revoked serial number ranges indexed by issuer public key hash.
  // The key is the SHA256 hash of issuer's SubjectPublicKeyInfo.
  // The value is a list of revoked serial number ranges.
  std::unordered_map<std::string, std::vector<SerialNumberRange>>
      revoked_serial_numbers_;

  OSP_DISALLOW_COPY_AND_ASSIGN(CastCRL);
};

struct TrustStore;

// Parses and verifies the CRL used to verify the revocation status of
// Cast device certificates, using the built-in Cast CRL trust anchors.
//
// Inputs:
// * |crl_proto| is a serialized cast_certificate.CrlBundle proto.
// * |time| is the timestamp to use for determining if the CRL is valid.
// * |trust_store| is the set of trust anchors to use.  This should be nullptr
//   in production, but can be overridden in tests.
//
// Output:
// Returns the CRL object if success, nullptr otherwise.
std::unique_ptr<CastCRL> ParseAndVerifyCRL(const std::string& crl_proto,
                                           const DateTime& time,
                                           TrustStore* trust_store = nullptr);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_CAST_CRL_H_
