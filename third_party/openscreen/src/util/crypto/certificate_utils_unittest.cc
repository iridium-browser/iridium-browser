// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/certificate_utils.h"

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <chrono>

#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "platform/base/error.h"
#include "util/std_util.h"

namespace openscreen {
namespace {

constexpr char kName[] = "test.com";
constexpr auto kDuration = std::chrono::seconds(31556952);

TEST(CertificateUtilTest, CreatesValidCertificate) {
  bssl::UniquePtr<EVP_PKEY> pkey = GenerateRsaKeyPair();
  ASSERT_TRUE(pkey);

  ErrorOr<bssl::UniquePtr<X509>> certificate =
      CreateSelfSignedX509Certificate(kName, kDuration, *pkey);
  ASSERT_TRUE(certificate.is_value());

  // Validate the generated certificate.
  EXPECT_NE(0, X509_verify(certificate.value().get(), pkey.get()));
}

TEST(CertificateUtilTest, ExportsAndImportsCertificate) {
  bssl::UniquePtr<EVP_PKEY> pkey = GenerateRsaKeyPair();
  ASSERT_TRUE(pkey);
  ErrorOr<bssl::UniquePtr<X509>> certificate =
      CreateSelfSignedX509Certificate(kName, kDuration, *pkey);
  ASSERT_TRUE(certificate.is_value());

  ErrorOr<std::vector<uint8_t>> exported =
      ExportX509CertificateToDer(*certificate.value());
  ASSERT_TRUE(exported.is_value()) << exported.error();
  EXPECT_FALSE(exported.value().empty());

  ErrorOr<bssl::UniquePtr<X509>> imported =
      ImportCertificate(exported.value().data(), exported.value().size());
  ASSERT_TRUE(imported.is_value()) << imported.error();
  ASSERT_TRUE(imported.value().get());

  // Validate the imported certificate.
  EXPECT_NE(0, X509_verify(imported.value().get(), pkey.get()));
}

}  // namespace
}  // namespace openscreen
