// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/cast_crl.h"

#include <time.h>

#include <memory>

#include "absl/strings/string_view.h"
#include "cast/common/certificate/date_time.h"
#include "cast/common/public/parsed_certificate.h"
#include "cast/common/public/trust_store.h"
#include "platform/base/macros.h"
#include "util/crypto/sha2.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

enum CrlVersion {
  // version 0: Spki Hash Algorithm = SHA-256
  //            Signature Algorithm = RSA-PKCS1 V1.5 with SHA-256
  kCrlVersion0 = 0,
};

ConstDataSpan ConstDataSpanFromString(const std::string& s) {
  return ConstDataSpan{reinterpret_cast<const uint8_t*>(s.data()),
                       static_cast<uint32_t>(s.size())};
}

// Verifies the CRL is signed by a trusted CRL authority at the time the CRL
// was issued. Verifies the signature of |tbs_crl| is valid based on the
// certificate and signature in |crl|. The validity of |tbs_crl| is verified
// at |time|. The validity period of the CRL is adjusted to be the earliest
// of the issuer certificate chain's expiration and the CRL's expiration and
// the result is stored in |overall_not_after|.
bool VerifyCRL(const Crl& crl,
               const TbsCrl& tbs_crl,
               const DateTime& time,
               TrustStore* trust_store,
               DateTime* overall_not_after) {
  ErrorOr<TrustStore::CertificatePathResult> maybe_result_path =
      trust_store->FindCertificatePath({crl.signer_cert()}, time);
  if (!maybe_result_path) {
    return false;
  }
  auto& result_path = maybe_result_path.value();
  ParsedCertificate* target_cert = result_path[0].get();

  if (!target_cert->VerifySignedData(
          DigestAlgorithm::kSha256, ConstDataSpanFromString(crl.tbs_crl()),
          ConstDataSpanFromString(crl.signature()))) {
    return false;
  }

  // Verify the CRL is still valid.
  DateTime not_before;
  if (!DateTimeFromSeconds(tbs_crl.not_before_seconds(), &not_before)) {
    return false;
  }
  DateTime not_after;
  if (!DateTimeFromSeconds(tbs_crl.not_after_seconds(), &not_after)) {
    return false;
  }
  if ((time < not_before) || (not_after < time)) {
    return false;
  }

  // Set CRL expiry to the earliest of the cert chain expiry and CRL expiry
  // (excluding trust anchor).  No intermediates are provided above, so this
  // just amounts to |signer_cert| vs. |not_after_seconds|.
  *overall_not_after = not_after;
  ErrorOr<DateTime> maybe_not_after = target_cert->GetNotAfterTime();
  if (!maybe_not_after) {
    return false;
  }
  DateTime& cert_not_after = maybe_not_after.value();
  if (cert_not_after < *overall_not_after) {
    *overall_not_after = cert_not_after;
  }

  // Perform sanity check on serial numbers.
  for (const auto& range : tbs_crl.revoked_serial_number_ranges()) {
    uint64_t first_serial_number = range.first_serial_number();
    uint64_t last_serial_number = range.last_serial_number();
    if (last_serial_number < first_serial_number) {
      return false;
    }
  }

  return true;
}

}  // namespace

CastCRL::CastCRL(const TbsCrl& tbs_crl, const DateTime& overall_not_after) {
  // Parse the validity information.
  // Assume DateTimeFromSeconds will succeed. Successful call to VerifyCRL means
  // that these calls were successful.
  DateTimeFromSeconds(tbs_crl.not_before_seconds(), &not_before_);
  DateTimeFromSeconds(tbs_crl.not_after_seconds(), &not_after_);
  if (overall_not_after < not_after_) {
    not_after_ = overall_not_after;
  }

  // Parse the revoked hashes.
  for (const auto& hash : tbs_crl.revoked_public_key_hashes()) {
    revoked_hashes_.insert(hash);
  }

  // Parse the revoked serial ranges.
  for (const auto& range : tbs_crl.revoked_serial_number_ranges()) {
    std::string issuer_hash = range.issuer_public_key_hash();

    uint64_t first_serial_number = range.first_serial_number();
    uint64_t last_serial_number = range.last_serial_number();
    auto& serial_number_range = revoked_serial_numbers_[issuer_hash];
    serial_number_range.push_back({first_serial_number, last_serial_number});
  }
}

CastCRL::~CastCRL() {}

// Verifies the revocation status of the certificate chain, at the specified
// time.
bool CastCRL::CheckRevocation(
    const std::vector<const ParsedCertificate*>& trusted_chain,
    const DateTime& time) const {
  if (trusted_chain.empty())
    return false;

  if ((time < not_before_) || (not_after_ < time)) {
    return false;
  }

  // Check revocation, starting from the trust anchor.
  for (size_t i = trusted_chain.size(); i > 0; --i) {
    size_t subject_index = i - 1;
    std::string spki_tlv = trusted_chain[subject_index]->GetSpkiTlv();
    if (spki_tlv.empty()) {
      return false;
    }

    ErrorOr<std::string> spki_hash = SHA256HashString(spki_tlv);
    if (spki_hash.is_error() ||
        (revoked_hashes_.find(spki_hash.value()) != revoked_hashes_.end())) {
      return false;
    }

    // Check if the subordinate certificate was revoked by serial number.
    if (subject_index > 0) {
      const auto issuer_iter = revoked_serial_numbers_.find(spki_hash.value());
      if (issuer_iter != revoked_serial_numbers_.end()) {
        const auto& subordinate = trusted_chain[subject_index - 1];
        uint64_t serial_number;

        // Only Google generated device certificates will be revoked by range.
        // These will always be less than 64 bits in length.
        ErrorOr<uint64_t> maybe_serial = subordinate->GetSerialNumber();
        if (!maybe_serial) {
          continue;
        }
        serial_number = maybe_serial.value();
        for (const auto& revoked_serial : issuer_iter->second) {
          if (revoked_serial.first_serial <= serial_number &&
              revoked_serial.last_serial >= serial_number) {
            return false;
          }
        }
      }
    }
  }
  return true;
}

std::unique_ptr<CastCRL> ParseAndVerifyCRL(const std::string& crl_proto,
                                           const DateTime& time,
                                           TrustStore* trust_store) {
  CrlBundle crl_bundle;
  if (!crl_bundle.ParseFromString(crl_proto)) {
    return nullptr;
  }
  for (const auto& crl : crl_bundle.crls()) {
    TbsCrl tbs_crl;
    if (!tbs_crl.ParseFromString(crl.tbs_crl())) {
      OSP_LOG_WARN << "Binary TBS CRL could not be parsed.";
      continue;
    }
    if (tbs_crl.version() != kCrlVersion0) {
      OSP_LOG_WARN << "Binary TBS CRL has unknown version: "
                   << tbs_crl.version();
      continue;
    }
    DateTime overall_not_after;
    if (!VerifyCRL(crl, tbs_crl, time, trust_store, &overall_not_after)) {
      return nullptr;
    }
    // TODO(btolsch): Why is this 'return first successful CRL'?
    return std::make_unique<CastCRL>(tbs_crl, overall_not_after);
  }
  return nullptr;
}

}  // namespace cast
}  // namespace openscreen
