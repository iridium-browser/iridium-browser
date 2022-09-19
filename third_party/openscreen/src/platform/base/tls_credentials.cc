// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/tls_credentials.h"

namespace openscreen {

TlsCredentials::TlsCredentials() = default;

TlsCredentials::TlsCredentials(std::vector<uint8_t> der_rsa_private_key,
                               std::vector<uint8_t> der_rsa_public_key,
                               std::vector<uint8_t> der_x509_cert)
    : der_rsa_private_key(std::move(der_rsa_private_key)),
      der_rsa_public_key(std::move(der_rsa_public_key)),
      der_x509_cert(std::move(der_x509_cert)) {}

TlsCredentials::~TlsCredentials() = default;

}  // namespace openscreen
