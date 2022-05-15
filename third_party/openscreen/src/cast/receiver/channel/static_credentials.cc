// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/receiver/channel/static_credentials.h"

#include <openssl/mem.h>
#include <openssl/pem.h>

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "platform/base/tls_credentials.h"
#include "util/crypto/certificate_utils.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {
using FileUniquePtr = std::unique_ptr<FILE, decltype(&fclose)>;

constexpr char kGeneratedRootCertificateName[] =
    "generated_root_cast_receiver.crt";
constexpr char kGeneratedPrivateKey[] = "generated_root_cast_receiver.key";
constexpr int kThreeDaysInSeconds = 3 * 24 * 60 * 60;
constexpr auto kCertificateDuration = std::chrono::seconds(kThreeDaysInSeconds);

ErrorOr<GeneratedCredentials> GenerateCredentials(
    std::string device_certificate_id,
    EVP_PKEY* root_key,
    X509* root_cert) {
  OSP_CHECK(root_key);
  OSP_CHECK(root_cert);
  bssl::UniquePtr<EVP_PKEY> intermediate_key = GenerateRsaKeyPair();
  bssl::UniquePtr<EVP_PKEY> device_key = GenerateRsaKeyPair();
  OSP_CHECK(intermediate_key);
  OSP_CHECK(device_key);

  ErrorOr<bssl::UniquePtr<X509>> intermediate_cert_or_error =
      CreateSelfSignedX509Certificate(
          "Cast Intermediate", kCertificateDuration, *intermediate_key,
          GetWallTimeSinceUnixEpoch(), true, root_cert, root_key);
  OSP_CHECK(intermediate_cert_or_error);
  bssl::UniquePtr<X509> intermediate_cert =
      std::move(intermediate_cert_or_error.value());

  ErrorOr<bssl::UniquePtr<X509>> device_cert_or_error =
      CreateSelfSignedX509Certificate(
          device_certificate_id, kCertificateDuration, *device_key,
          GetWallTimeSinceUnixEpoch(), false, intermediate_cert.get(),
          intermediate_key.get());
  OSP_CHECK(device_cert_or_error);
  bssl::UniquePtr<X509> device_cert = std::move(device_cert_or_error.value());

  // Device cert chain plumbing + serialization.
  DeviceCredentials device_creds;
  device_creds.private_key = std::move(device_key);

  int cert_length = i2d_X509(device_cert.get(), nullptr);
  std::string cert_serial(cert_length, 0);
  uint8_t* out = reinterpret_cast<uint8_t*>(&cert_serial[0]);
  i2d_X509(device_cert.get(), &out);
  device_creds.certs.emplace_back(std::move(cert_serial));

  cert_length = i2d_X509(intermediate_cert.get(), nullptr);
  cert_serial.resize(cert_length);
  out = reinterpret_cast<uint8_t*>(&cert_serial[0]);
  i2d_X509(intermediate_cert.get(), &out);
  device_creds.certs.emplace_back(std::move(cert_serial));

  cert_length = i2d_X509(root_cert, nullptr);
  std::vector<uint8_t> trust_anchor_der(cert_length);
  out = &trust_anchor_der[0];
  i2d_X509(root_cert, &out);

  // TLS key pair + certificate generation.
  bssl::UniquePtr<EVP_PKEY> tls_key = GenerateRsaKeyPair();
  OSP_CHECK_EQ(EVP_PKEY_id(tls_key.get()), EVP_PKEY_RSA);
  ErrorOr<bssl::UniquePtr<X509>> tls_cert_or_error =
      CreateSelfSignedX509Certificate("Test Device TLS", kCertificateDuration,
                                      *tls_key, GetWallTimeSinceUnixEpoch());
  OSP_CHECK(tls_cert_or_error);
  bssl::UniquePtr<X509> tls_cert = std::move(tls_cert_or_error.value());

  // TLS private key serialization.
  RSA* rsa_key = EVP_PKEY_get0_RSA(tls_key.get());
  size_t pkey_len = 0;
  uint8_t* pkey_bytes = nullptr;
  OSP_CHECK(RSA_private_key_to_bytes(&pkey_bytes, &pkey_len, rsa_key));
  OSP_CHECK_GT(pkey_len, 0u);
  std::vector<uint8_t> tls_key_serial(pkey_bytes, pkey_bytes + pkey_len);
  OPENSSL_free(pkey_bytes);

  // TLS public key serialization.
  pkey_len = 0;
  pkey_bytes = nullptr;
  OSP_CHECK(RSA_public_key_to_bytes(&pkey_bytes, &pkey_len, rsa_key));
  OSP_CHECK_GT(pkey_len, 0u);
  std::vector<uint8_t> tls_pub_serial(pkey_bytes, pkey_bytes + pkey_len);
  OPENSSL_free(pkey_bytes);

  // TLS cert serialization.
  cert_length = 0;
  cert_length = i2d_X509(tls_cert.get(), nullptr);
  OSP_CHECK_GT(cert_length, 0);
  std::vector<uint8_t> tls_cert_serial(cert_length);
  out = &tls_cert_serial[0];
  i2d_X509(tls_cert.get(), &out);

  auto provider = std::make_unique<StaticCredentialsProvider>(
      std::move(device_creds), tls_cert_serial);
  return GeneratedCredentials{
      std::move(provider),
      TlsCredentials{std::move(tls_key_serial), std::move(tls_pub_serial),
                     std::move(tls_cert_serial)},
      std::move(trust_anchor_der)};
}

bssl::UniquePtr<EVP_PKEY> GeneratePrivateKey() {
  bssl::UniquePtr<EVP_PKEY> root_key = GenerateRsaKeyPair();
  OSP_CHECK(root_key);
  return root_key;
}

bssl::UniquePtr<X509> GenerateRootCert(const EVP_PKEY& root_key) {
  ErrorOr<bssl::UniquePtr<X509>> root_cert_or_error =
      CreateSelfSignedX509Certificate("Cast Root CA", kCertificateDuration,
                                      root_key, GetWallTimeSinceUnixEpoch(),
                                      true);
  OSP_CHECK(root_cert_or_error);
  return std::move(root_cert_or_error.value());
}
}  // namespace

StaticCredentialsProvider::StaticCredentialsProvider() = default;
StaticCredentialsProvider::StaticCredentialsProvider(
    DeviceCredentials device_creds,
    std::vector<uint8_t> tls_cert_der)
    : device_creds(std::move(device_creds)),
      tls_cert_der(std::move(tls_cert_der)) {}

StaticCredentialsProvider::StaticCredentialsProvider(
    StaticCredentialsProvider&&) noexcept = default;
StaticCredentialsProvider& StaticCredentialsProvider::operator=(
    StaticCredentialsProvider&&) = default;
StaticCredentialsProvider::~StaticCredentialsProvider() = default;

void GenerateDeveloperCredentialsToFile() {
  bssl::UniquePtr<EVP_PKEY> root_key = GeneratePrivateKey();
  bssl::UniquePtr<X509> root_cert = GenerateRootCert(*root_key);

  FileUniquePtr f(fopen(kGeneratedPrivateKey, "w"), &fclose);
  if (PEM_write_PrivateKey(f.get(), root_key.get(), nullptr, nullptr, 0, 0,
                           nullptr) != 1) {
    OSP_LOG_ERROR << "Failed to write private key, check permissions?";
    return;
  }
  OSP_LOG_INFO << "Generated new private key for session: ./"
               << kGeneratedPrivateKey;

  FileUniquePtr cert_file(fopen(kGeneratedRootCertificateName, "w"), &fclose);
  if (PEM_write_X509(cert_file.get(), root_cert.get()) != 1) {
    OSP_LOG_ERROR << "Failed to write root certificate, check permissions?";
    return;
  }
  OSP_LOG_INFO << "Generated new root certificate for session: ./"
               << kGeneratedRootCertificateName;
}

ErrorOr<GeneratedCredentials> GenerateCredentialsForTesting(
    const std::string& device_certificate_id) {
  bssl::UniquePtr<EVP_PKEY> root_key = GeneratePrivateKey();
  bssl::UniquePtr<X509> root_cert = GenerateRootCert(*root_key);
  return GenerateCredentials(device_certificate_id, root_key.get(),
                             root_cert.get());
}

ErrorOr<GeneratedCredentials> GenerateCredentials(
    const std::string& device_certificate_id,
    const std::string& private_key_path,
    const std::string& server_certificate_path) {
  if (private_key_path.empty() || server_certificate_path.empty()) {
    return Error(Error::Code::kParameterInvalid,
                 "Missing either private key or server certificate");
  }

  FileUniquePtr key_file(fopen(private_key_path.c_str(), "r"), &fclose);
  if (!key_file) {
    return Error(Error::Code::kParameterInvalid,
                 "Missing private key file path");
  }
  bssl::UniquePtr<EVP_PKEY> root_key =
      bssl::UniquePtr<EVP_PKEY>(PEM_read_PrivateKey(
          key_file.get(), nullptr /* x */, nullptr /* cb */, nullptr /* u */));

  FileUniquePtr cert_file(fopen(server_certificate_path.c_str(), "r"), &fclose);
  if (!cert_file) {
    return Error(Error::Code::kParameterInvalid,
                 "Missing server certificate file path");
  }
  bssl::UniquePtr<X509> root_cert = bssl::UniquePtr<X509>(PEM_read_X509(
      cert_file.get(), nullptr /* x */, nullptr /* cb */, nullptr /* u */));

  return GenerateCredentials(device_certificate_id, root_key.get(),
                             root_cert.get());
}

}  // namespace cast
}  // namespace openscreen
