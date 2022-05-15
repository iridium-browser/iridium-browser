// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/boringssl_parsed_certificate.h"

#include <openssl/x509v3.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "cast/common/certificate/boringssl_util.h"
#include "util/crypto/certificate_utils.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

const EVP_MD* MapDigestAlgorithm(DigestAlgorithm algorithm) {
  switch (algorithm) {
    case DigestAlgorithm::kSha1:
      return EVP_sha1();
    case DigestAlgorithm::kSha256:
      return EVP_sha256();
    case DigestAlgorithm::kSha384:
      return EVP_sha384();
    case DigestAlgorithm::kSha512:
      return EVP_sha512();
    default:
      return nullptr;
  }
}

}  // namespace

// static
ErrorOr<std::unique_ptr<ParsedCertificate>> ParsedCertificate::ParseFromDER(
    const std::vector<uint8_t>& der_cert) {
  ErrorOr<bssl::UniquePtr<X509>> parsed =
      ImportCertificate(der_cert.data(), der_cert.size());
  if (!parsed) {
    return parsed.error();
  }
  std::unique_ptr<ParsedCertificate> result =
      std::make_unique<BoringSSLParsedCertificate>(std::move(parsed.value()));
  return result;
}

BoringSSLParsedCertificate::BoringSSLParsedCertificate() = default;

BoringSSLParsedCertificate::BoringSSLParsedCertificate(
    bssl::UniquePtr<X509> cert)
    : cert_(std::move(cert)) {}

BoringSSLParsedCertificate::~BoringSSLParsedCertificate() = default;

ErrorOr<std::vector<uint8_t>> BoringSSLParsedCertificate::SerializeToDER(
    int front_spacing) const {
  OSP_DCHECK_GE(front_spacing, 0);
  int cert_len = i2d_X509(cert_.get(), nullptr);
  if (cert_len <= 0) {
    return Error(Error::Code::kErrCertSerialize, "Serializing cert failed.");
  }
  std::vector<uint8_t> cert_der(front_spacing + cert_len, 0);
  uint8_t* data = &cert_der[front_spacing];
  if (!i2d_X509(cert_.get(), &data)) {
    return Error(Error::Code::kErrCertSerialize, "Serializing cert failed.");
  }
  return cert_der;
}

ErrorOr<DateTime> BoringSSLParsedCertificate::GetNotBeforeTime() const {
  return openscreen::cast::GetNotBeforeTime(cert_.get());
}

ErrorOr<DateTime> BoringSSLParsedCertificate::GetNotAfterTime() const {
  return openscreen::cast::GetNotAfterTime(cert_.get());
}

std::string BoringSSLParsedCertificate::GetCommonName() const {
  X509_NAME* target_subject = X509_get_subject_name(cert_.get());
  int len =
      X509_NAME_get_text_by_NID(target_subject, NID_commonName, nullptr, 0);
  if (len <= 0) {
    return {};
  }
  // X509_NAME_get_text_by_NID writes one more byte than it reports, for a
  // trailing NUL.
  std::string common_name(len + 1, 0);
  len = X509_NAME_get_text_by_NID(target_subject, NID_commonName,
                                  &common_name[0], common_name.size());
  if (len <= 0) {
    return {};
  }
  common_name.resize(len);
  return common_name;
}

std::string BoringSSLParsedCertificate::GetSpkiTlv() const {
  return openscreen::GetSpkiTlv(cert_.get());
}

ErrorOr<uint64_t> BoringSSLParsedCertificate::GetSerialNumber() const {
  return ParseDerUint64(X509_get0_serialNumber(cert_.get()));
}

bool BoringSSLParsedCertificate::VerifySignedData(
    DigestAlgorithm algorithm,
    const ConstDataSpan& data,
    const ConstDataSpan& signature) const {
  const EVP_MD* digest = MapDigestAlgorithm(algorithm);
  if (!digest) {
    return false;
  }

  bssl::UniquePtr<EVP_PKEY> public_key{X509_get_pubkey(cert_.get())};
  return openscreen::cast::VerifySignedData(digest, public_key.get(), data,
                                            signature);
}

bool BoringSSLParsedCertificate::HasPolicyOid(const ConstDataSpan& oid) const {
  // Gets an index into the extension list that points to the
  // certificatePolicies extension, if it exists, -1 otherwise.
  bool has_oid = false;
  int pos = X509_get_ext_by_NID(cert_.get(), NID_certificate_policies, -1);
  if (pos != -1) {
    X509_EXTENSION* policies_extension = X509_get_ext(cert_.get(), pos);
    if (!policies_extension) {
      return false;
    }
    const ASN1_STRING* value = X509_EXTENSION_get_data(policies_extension);
    if (!value) {
      return false;
    }
    const uint8_t* in = ASN1_STRING_get0_data(value);
    if (!in) {
      return false;
    }
    CERTIFICATEPOLICIES* policies =
        d2i_CERTIFICATEPOLICIES(nullptr, &in, ASN1_STRING_length(value));

    if (policies) {
      // Check for |oid| in the set of policies.
      uint32_t policy_count = sk_POLICYINFO_num(policies);
      for (uint32_t i = 0; i < policy_count; ++i) {
        POLICYINFO* info = sk_POLICYINFO_value(policies, i);
        if (OBJ_length(info->policyid) == oid.length &&
            memcmp(OBJ_get0_data(info->policyid), oid.data, oid.length) == 0) {
          has_oid = true;
          break;
        }
      }
      CERTIFICATEPOLICIES_free(policies);
    }
  }
  return has_oid;
}

void BoringSSLParsedCertificate::SetNotBeforeTimeForTesting(time_t not_before) {
  ASN1_TIME_set(const_cast<ASN1_TIME*>(X509_get0_notBefore(cert_.get())),
                not_before);
}

void BoringSSLParsedCertificate::SetNotAfterTimeForTesting(time_t not_after) {
  ASN1_TIME_set(const_cast<ASN1_TIME*>(X509_get0_notAfter(cert_.get())),
                not_after);
}

}  // namespace cast
}  // namespace openscreen
