// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_RECEIVER_CHANNEL_STATIC_CREDENTIALS_H_
#define CAST_RECEIVER_CHANNEL_STATIC_CREDENTIALS_H_

#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "cast/receiver/channel/device_auth_namespace_handler.h"
#include "platform/base/error.h"
#include "platform/base/tls_credentials.h"

namespace openscreen {
namespace cast {

class StaticCredentialsProvider final
    : public DeviceAuthNamespaceHandler::CredentialsProvider {
 public:
  StaticCredentialsProvider();
  StaticCredentialsProvider(DeviceCredentials device_creds,
                            std::vector<uint8_t> tls_cert_der);

  StaticCredentialsProvider(const StaticCredentialsProvider&) = delete;
  StaticCredentialsProvider(StaticCredentialsProvider&&) noexcept;
  StaticCredentialsProvider& operator=(const StaticCredentialsProvider&) =
      delete;
  StaticCredentialsProvider& operator=(StaticCredentialsProvider&&);
  ~StaticCredentialsProvider();

  absl::Span<const uint8_t> GetCurrentTlsCertAsDer() override {
    return absl::Span<uint8_t>(tls_cert_der);
  }
  const DeviceCredentials& GetCurrentDeviceCredentials() override {
    return device_creds;
  }

  DeviceCredentials device_creds;
  std::vector<uint8_t> tls_cert_der;
};

struct GeneratedCredentials {
  std::unique_ptr<StaticCredentialsProvider> provider;
  TlsCredentials tls_credentials;
  std::vector<uint8_t> root_cert_der;
};

// Generates a private key and root TLS server certificate for use with cast
// sockets.
void GenerateDeveloperCredentialsToFile();

// Generates a valid set of credentials for use with cast sockets/TLS.
// Both the private key and server certificate paths are required, except
// in testing where they can be omitted.
ErrorOr<GeneratedCredentials> GenerateCredentials(
    const std::string& device_certificate_id,
    const std::string& private_key_path,
    const std::string& server_certificate_path);

ErrorOr<GeneratedCredentials> GenerateCredentialsForTesting(
    const std::string& device_certificate_id);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_RECEIVER_CHANNEL_STATIC_CREDENTIALS_H_
