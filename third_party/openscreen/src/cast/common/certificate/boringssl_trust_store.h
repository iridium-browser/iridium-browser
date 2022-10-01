// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_BORINGSSL_TRUST_STORE_H_
#define CAST_COMMON_CERTIFICATE_BORINGSSL_TRUST_STORE_H_

#include <openssl/x509.h>

#include <cstdint>
#include <string>
#include <vector>

#include "cast/common/public/trust_store.h"

namespace openscreen {
namespace cast {

class BoringSSLTrustStore final : public TrustStore {
 public:
  BoringSSLTrustStore();
  explicit BoringSSLTrustStore(const std::vector<uint8_t>& trust_anchor_der);
  explicit BoringSSLTrustStore(std::vector<bssl::UniquePtr<X509>> certs);
  ~BoringSSLTrustStore() override;

  ErrorOr<CertificatePathResult> FindCertificatePath(
      const std::vector<std::string>& der_certs,
      const DateTime& time) override;

 private:
  std::vector<bssl::UniquePtr<X509>> certs_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_BORINGSSL_TRUST_STORE_H_
