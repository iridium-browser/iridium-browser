// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_BORINGSSL_UTIL_H_
#define CAST_COMMON_CERTIFICATE_BORINGSSL_UTIL_H_

#include <openssl/evp.h>
#include <openssl/x509v3.h>

#include "cast/common/public/certificate_types.h"
#include "platform/base/error.h"

namespace openscreen {
namespace cast {

bool VerifySignedData(const EVP_MD* digest,
                      EVP_PKEY* public_key,
                      const ConstDataSpan& data,
                      const ConstDataSpan& signature);

ErrorOr<DateTime> GetNotBeforeTime(X509* cert);
ErrorOr<DateTime> GetNotAfterTime(X509* cert);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_BORINGSSL_UTIL_H_
