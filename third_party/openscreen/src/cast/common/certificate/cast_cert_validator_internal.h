// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_CAST_CERT_VALIDATOR_INTERNAL_H_
#define CAST_COMMON_CERTIFICATE_CAST_CERT_VALIDATOR_INTERNAL_H_

#include <openssl/x509.h>

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "platform/base/error.h"
namespace openscreen {
namespace cast {

struct TrustStore {
  enum class Mode {
    // In strict mode, only certificates signed by a CA will be accepted as
    // part of authentication. Note that if a self-signed certificate is placed
    // in a strict mode TrustStore, it cannot be used for authentication.
    kStrict,

    // In allow self signed mode, certificates signed by an arbitrary private
    // key that have been placed in this trust store will be allowed. Note
    // that certificates must still otherwise be valid.
    kAllowSelfSigned
  };

  static TrustStore CreateInstanceFromPemFile(absl::string_view file_path);

  std::vector<bssl::UniquePtr<X509>> certs;
};

// Adds a trust anchor given a DER-encoded certificate from static
// storage.
template <size_t N>
bssl::UniquePtr<X509> MakeTrustAnchor(const uint8_t (&data)[N]) {
  const uint8_t* dptr = data;
  return bssl::UniquePtr<X509>{d2i_X509(nullptr, &dptr, N)};
}

inline bssl::UniquePtr<X509> MakeTrustAnchor(const std::vector<uint8_t>& data) {
  const uint8_t* dptr = data.data();
  return bssl::UniquePtr<X509>{d2i_X509(nullptr, &dptr, data.size())};
}

struct ConstDataSpan;
struct DateTime;

bool VerifySignedData(const EVP_MD* digest,
                      EVP_PKEY* public_key,
                      const ConstDataSpan& data,
                      const ConstDataSpan& signature);

// Parses DateTime with additional restrictions laid out by RFC 5280
// 4.1.2.5.2.
bool ParseAsn1GeneralizedTime(ASN1_GENERALIZEDTIME* time, DateTime* out);
bool GetCertValidTimeRange(X509* cert,
                           DateTime* not_before,
                           DateTime* not_after);

struct CertificatePathResult {
  bssl::UniquePtr<X509> target_cert;
  std::vector<bssl::UniquePtr<X509>> intermediate_certs;
  std::vector<X509*> path;
};

Error FindCertificatePath(const std::vector<std::string>& der_certs,
                          const DateTime& time,
                          CertificatePathResult* result_path,
                          TrustStore* trust_store);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_CAST_CERT_VALIDATOR_INTERNAL_H_
