// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/receiver/channel/device_auth_namespace_handler.h"

#include <openssl/evp.h>

#include <memory>
#include <utility>

#include "cast/common/certificate/cast_cert_validator.h"
#include "cast/common/channel/message_util.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/channel/virtual_connection.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "platform/base/tls_credentials.h"
#include "util/crypto/digest_sign.h"

using ::cast::channel::AuthChallenge;
using ::cast::channel::AuthError;
using ::cast::channel::AuthResponse;
using ::cast::channel::CastMessage;
using ::cast::channel::DeviceAuthMessage;
using ::cast::channel::HashAlgorithm;
using ::cast::channel::SignatureAlgorithm;

namespace openscreen {
namespace cast {

namespace {

CastMessage GenerateErrorMessage(AuthError::ErrorType error_type) {
  DeviceAuthMessage message;
  AuthError* error = message.mutable_error();
  error->set_error_type(error_type);
  std::string payload;
  message.SerializeToString(&payload);

  CastMessage response;
  response.set_protocol_version(
      ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_0);
  response.set_namespace_(kAuthNamespace);
  response.set_payload_type(::cast::channel::CastMessage_PayloadType_BINARY);
  response.set_payload_binary(std::move(payload));
  return response;
}

}  // namespace

DeviceAuthNamespaceHandler::CredentialsProvider::~CredentialsProvider() =
    default;

DeviceAuthNamespaceHandler::DeviceAuthNamespaceHandler(
    CredentialsProvider* creds_provider)
    : creds_provider_(creds_provider) {}

DeviceAuthNamespaceHandler::~DeviceAuthNamespaceHandler() = default;

void DeviceAuthNamespaceHandler::OnMessage(VirtualConnectionRouter* router,
                                           CastSocket* socket,
                                           CastMessage message) {
  if (!socket) {
    return;  // Don't handle auth messages from local senders. That's nonsense.
  }
  if (message.payload_type() !=
      ::cast::channel::CastMessage_PayloadType_BINARY) {
    return;
  }
  const std::string& payload = message.payload_binary();
  DeviceAuthMessage device_auth_message;
  if (!device_auth_message.ParseFromArray(payload.data(), payload.length())) {
    // TODO(btolsch): Consider all of these cases for future error reporting
    // mechanism.
    return;
  }

  if (!device_auth_message.has_challenge()) {
    return;
  }

  if (device_auth_message.has_response() || device_auth_message.has_error()) {
    return;
  }

  const VirtualConnection virtual_conn{
      message.destination_id(), message.source_id(), socket->socket_id()};
  const AuthChallenge& challenge = device_auth_message.challenge();
  const SignatureAlgorithm sig_alg = challenge.signature_algorithm();
  HashAlgorithm hash_alg = challenge.hash_algorithm();
  // TODO(btolsch): Reconsider supporting SHA1 after further metrics
  // investigation.
  if ((sig_alg != ::cast::channel::UNSPECIFIED &&
       sig_alg != ::cast::channel::RSASSA_PKCS1v15) ||
      (hash_alg != ::cast::channel::SHA1 &&
       hash_alg != ::cast::channel::SHA256)) {
    router->Send(virtual_conn, GenerateErrorMessage(
                                   AuthError::SIGNATURE_ALGORITHM_UNAVAILABLE));
    return;
  }
  const EVP_MD* digest =
      hash_alg == ::cast::channel::SHA256 ? EVP_sha256() : EVP_sha1();

  const absl::Span<const uint8_t> tls_cert_der =
      creds_provider_->GetCurrentTlsCertAsDer();
  const DeviceCredentials& device_creds =
      creds_provider_->GetCurrentDeviceCredentials();
  if (tls_cert_der.empty() || device_creds.certs.empty() ||
      !device_creds.private_key) {
    // TODO(btolsch): Add this to future error reporting.
    router->Send(virtual_conn, GenerateErrorMessage(AuthError::INTERNAL_ERROR));
    return;
  }

  std::unique_ptr<AuthResponse> auth_response(new AuthResponse());
  auth_response->set_client_auth_certificate(device_creds.certs[0]);
  for (auto it = device_creds.certs.begin() + 1; it != device_creds.certs.end();
       ++it) {
    auth_response->add_intermediate_certificate(*it);
  }
  auth_response->set_signature_algorithm(::cast::channel::RSASSA_PKCS1v15);
  auth_response->set_hash_algorithm(hash_alg);
  std::string sender_nonce;
  if (challenge.has_sender_nonce()) {
    sender_nonce = challenge.sender_nonce();
    auth_response->set_sender_nonce(sender_nonce);
  }

  auth_response->set_crl(device_creds.serialized_crl);

  std::vector<uint8_t> to_be_signed;
  to_be_signed.reserve(sender_nonce.size() + tls_cert_der.size());
  to_be_signed.insert(to_be_signed.end(), sender_nonce.begin(),
                      sender_nonce.end());
  to_be_signed.insert(to_be_signed.end(), tls_cert_der.begin(),
                      tls_cert_der.end());

  ErrorOr<std::string> signature =
      SignData(digest, device_creds.private_key.get(), to_be_signed);
  if (!signature) {
    router->Send(virtual_conn, GenerateErrorMessage(AuthError::INTERNAL_ERROR));
    return;
  }
  auth_response->set_signature(std::move(signature.value()));

  DeviceAuthMessage response_auth_message;
  response_auth_message.set_allocated_response(auth_response.release());

  std::string response_string;
  response_auth_message.SerializeToString(&response_string);
  CastMessage response;
  response.set_protocol_version(
      ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_0);
  response.set_namespace_(kAuthNamespace);
  response.set_payload_type(::cast::channel::CastMessage_PayloadType_BINARY);
  response.set_payload_binary(std::move(response_string));
  router->Send(virtual_conn, std::move(response));
}

}  // namespace cast
}  // namespace openscreen
