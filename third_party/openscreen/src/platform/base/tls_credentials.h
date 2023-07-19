// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_TLS_CREDENTIALS_H_
#define PLATFORM_BASE_TLS_CREDENTIALS_H_

#include <stdint.h>

#include <vector>

namespace openscreen {

struct TlsCredentials {
  TlsCredentials();
  TlsCredentials(std::vector<uint8_t> der_rsa_private_key,
                 std::vector<uint8_t> der_rsa_public_key,
                 std::vector<uint8_t> der_x509_cert);
  ~TlsCredentials();

  // DER-encoded RSA private key.
  std::vector<uint8_t> der_rsa_private_key;

  // DER-encoded RSA public key.
  std::vector<uint8_t> der_rsa_public_key;

  // DER-encoded X509 Certificate that is based on the above keys.
  std::vector<uint8_t> der_x509_cert;
};

}  // namespace openscreen

#endif  // PLATFORM_BASE_TLS_CREDENTIALS_H_
