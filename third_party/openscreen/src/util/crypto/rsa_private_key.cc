// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/rsa_private_key.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "openssl/bn.h"
#include "openssl/bytestring.h"
#include "openssl/evp.h"
#include "openssl/mem.h"
#include "openssl/rsa.h"
#include "util/crypto/openssl_util.h"
#include "util/osp_logging.h"

namespace openscreen {
RSAPrivateKey::~RSAPrivateKey() = default;

// static
ErrorOr<RSAPrivateKey> RSAPrivateKey::Create(uint16_t num_bits) {
  OpenSSLErrStackTracer err_tracer(CURRENT_LOCATION);

  bssl::UniquePtr<RSA> rsa_key(RSA_new());
  bssl::UniquePtr<BIGNUM> exponent(BN_new());
  if (!rsa_key.get() || !exponent.get() ||
      !BN_set_word(exponent.get(), 65537L) ||
      !RSA_generate_key_ex(rsa_key.get(), num_bits, exponent.get(), nullptr)) {
    return Error::Code::kRSAKeyGenerationFailure;
  }

  RSAPrivateKey result;
  result.key_.reset(EVP_PKEY_new());
  if (!result.key_ || !EVP_PKEY_set1_RSA(result.key_.get(), rsa_key.get())) {
    return Error::Code::kEVPInitializationError;
  }

  return result;
}

// static
ErrorOr<RSAPrivateKey> RSAPrivateKey::CreateFromPrivateKeyInfo(
    const std::vector<uint8_t>& input) {
  OpenSSLErrStackTracer err_tracer(CURRENT_LOCATION);

  CBS private_key_cbs;
  CBS_init(&private_key_cbs, input.data(), input.size());
  bssl::UniquePtr<EVP_PKEY> private_key(
      EVP_parse_private_key(&private_key_cbs));
  if (!private_key || CBS_len(&private_key_cbs) != 0 ||
      EVP_PKEY_id(private_key.get()) != EVP_PKEY_RSA) {
    return Error::Code::kEVPInitializationError;
  }

  RSAPrivateKey result;
  result.key_ = std::move(private_key);
  return result;
}

// static
ErrorOr<RSAPrivateKey> RSAPrivateKey::CreateFromKey(EVP_PKEY* key) {
  OpenSSLErrStackTracer err_tracer(CURRENT_LOCATION);
  OSP_DCHECK(key);
  if (EVP_PKEY_id(key) != EVP_PKEY_RSA) {
    return Error::Code::kEVPInitializationError;
  }

  RSAPrivateKey result;
  result.key_ = bssl::UpRef(key);
  return result;
}

ErrorOr<RSAPrivateKey> RSAPrivateKey::Copy() const {
  OpenSSLErrStackTracer err_tracer(CURRENT_LOCATION);
  RSAPrivateKey result;
  bssl::UniquePtr<RSA> rsa(EVP_PKEY_get1_RSA(key_.get()));
  if (!rsa) {
    return Error::Code::kEVPInitializationError;
  }

  result.key_.reset(EVP_PKEY_new());
  if (!EVP_PKEY_set1_RSA(result.key_.get(), rsa.get())) {
    return Error::Code::kEVPInitializationError;
  }

  return result;
}

ErrorOr<std::vector<uint8_t>> RSAPrivateKey::ExportPrivateKey() const {
  OpenSSLErrStackTracer err_tracer(CURRENT_LOCATION);
  uint8_t* der;
  size_t der_len;
  bssl::ScopedCBB cbb;
  if (!CBB_init(cbb.get(), 0) ||
      !EVP_marshal_private_key(cbb.get(), key_.get()) ||
      !CBB_finish(cbb.get(), &der, &der_len)) {
    return Error::Code::kParseError;
  }

  std::vector<uint8_t> output(der, der + der_len);
  // The temporary buffer has to be freed after we properly copy out data.
  OPENSSL_free(der);
  return output;
}

ErrorOr<std::vector<uint8_t>> RSAPrivateKey::ExportPublicKey() const {
  OpenSSLErrStackTracer err_tracer(CURRENT_LOCATION);
  uint8_t* der;
  size_t der_len;
  bssl::ScopedCBB cbb;
  if (!CBB_init(cbb.get(), 0) ||
      !EVP_marshal_public_key(cbb.get(), key_.get()) ||
      !CBB_finish(cbb.get(), &der, &der_len)) {
    return Error::Code::kParseError;
  }

  std::vector<uint8_t> output(der, der + der_len);
  // The temporary buffer has to be freed after we properly copy out data.
  OPENSSL_free(der);
  return output;
}

RSAPrivateKey::RSAPrivateKey() = default;

}  // namespace openscreen
