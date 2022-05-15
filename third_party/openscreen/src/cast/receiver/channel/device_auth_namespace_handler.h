// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_RECEIVER_CHANNEL_DEVICE_AUTH_NAMESPACE_HANDLER_H_
#define CAST_RECEIVER_CHANNEL_DEVICE_AUTH_NAMESPACE_HANDLER_H_

#include <openssl/evp.h>

#include <string>
#include <vector>

#include "absl/types/span.h"
#include "cast/common/channel/cast_message_handler.h"

namespace openscreen {
namespace cast {

struct DeviceCredentials {
  // The device's certificate chain in DER form, where |certs[0]| is the
  // device's certificate and |certs[certs.size()-1]| is the last intermediate
  // before a Cast root certificate.
  std::vector<std::string> certs;

  // The device's private key that corresponds to the certificate in |certs[0]|.
  bssl::UniquePtr<EVP_PKEY> private_key;

  // If non-empty, this contains a serialized CrlBundle protobuf.  This may be
  // used by the sender as part of verifying |certs|.
  std::string serialized_crl;
};

class DeviceAuthNamespaceHandler final : public CastMessageHandler {
 public:
  class CredentialsProvider {
   public:
    virtual absl::Span<const uint8_t> GetCurrentTlsCertAsDer() = 0;
    virtual const DeviceCredentials& GetCurrentDeviceCredentials() = 0;
  };

  // |creds_provider| must outlive |this|.
  explicit DeviceAuthNamespaceHandler(CredentialsProvider* creds_provider);
  ~DeviceAuthNamespaceHandler();

  // CastMessageHandler overrides.
  void OnMessage(VirtualConnectionRouter* router,
                 CastSocket* socket,
                 ::cast::channel::CastMessage message) override;

 private:
  CredentialsProvider* const creds_provider_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_RECEIVER_CHANNEL_DEVICE_AUTH_NAMESPACE_HANDLER_H_
