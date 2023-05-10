// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/boringssl_trust_store.h"

#include <openssl/asn1.h>
#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <time.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "absl/strings/str_cat.h"
#include "cast/common/certificate/boringssl_parsed_certificate.h"
#include "cast/common/certificate/boringssl_util.h"
#include "cast/common/certificate/date_time.h"
#include "util/crypto/pem_helpers.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

// -------------------------------------------------------------------------
// Cast trust anchors.
// -------------------------------------------------------------------------

// There are two trusted roots for Cast certificate chains:
//
//   (1) CN=Cast Root CA    (kCastRootCaDer)
//   (2) CN=Eureka Root CA  (kEurekaRootCaDer)
//
// These constants are defined by the files included next:

#include "cast/common/certificate/cast_root_ca_cert_der-inc.h"
#include "cast/common/certificate/eureka_root_ca_der-inc.h"

// -------------------------------------------------------------------------
// Cast CRL trust anchors.
// -------------------------------------------------------------------------

// There is one trusted root for Cast CRL certificate chains:
//
//   (1) CN=Cast CRL Root CA    (kCastCRLRootCaDer)
//
// These constants are defined by the file included next:

#include "cast/common/certificate/cast_crl_root_ca_cert_der-inc.h"

// Adds a trust anchor given a DER-encoded certificate from static
// storage.
template <size_t N>
bssl::UniquePtr<X509> MakeTrustAnchor(const uint8_t (&data)[N]) {
  const uint8_t* dptr = data;
  return bssl::UniquePtr<X509>{d2i_X509(nullptr, &dptr, N)};
}

inline bssl::UniquePtr<X509> MakeTrustAnchor(const std::vector<uint8_t>& data) {
  const uint8_t* dptr = data.data();
  return bssl::UniquePtr<X509>{d2i_X509(nullptr, &dptr, data.size())};
}

constexpr static int32_t kMinRsaModulusLengthBits = 2048;

// TODO(davidben): Switch this to bssl::UniquePtr after
// https://boringssl-review.googlesource.com/c/boringssl/+/46105 lands.
struct FreeNameConstraints {
  void operator()(NAME_CONSTRAINTS* nc) { NAME_CONSTRAINTS_free(nc); }
};
using NameConstraintsPtr =
    std::unique_ptr<NAME_CONSTRAINTS, FreeNameConstraints>;

// Stores intermediate state while attempting to find a valid certificate chain
// from a set of trusted certificates to a target certificate.  Together, a
// sequence of these forms a certificate chain to be verified as well as a stack
// that can be unwound for searching more potential paths.
struct CertPathStep {
  X509* cert;

  // The next index that can be checked in |trust_store| if the choice |cert| on
  // the path needs to be reverted.
  uint32_t trust_store_index;

  // The next index that can be checked in |intermediate_certs| if the choice
  // |cert| on the path needs to be reverted.
  uint32_t intermediate_cert_index;
};

// These values are bit positions from RFC 5280 4.2.1.3 and will be passed to
// ASN1_BIT_STRING_get_bit.
enum KeyUsageBits {
  kDigitalSignature = 0,
  kKeyCertSign = 5,
};

bool CertInPath(X509_NAME* name,
                const std::vector<CertPathStep>& steps,
                uint32_t start,
                uint32_t stop) {
  for (uint32_t i = start; i < stop; ++i) {
    if (X509_NAME_cmp(name, X509_get_subject_name(steps[i].cert)) == 0) {
      return true;
    }
  }
  return false;
}

bssl::UniquePtr<BASIC_CONSTRAINTS> GetConstraints(X509* issuer) {
  // TODO(davidben): This and other |X509_get_ext_d2i| are missing
  // error-handling for syntax errors in certificates. See BoringSSL
  // documentation for the calling convention.
  return bssl::UniquePtr<BASIC_CONSTRAINTS>{
      reinterpret_cast<BASIC_CONSTRAINTS*>(
          X509_get_ext_d2i(issuer, NID_basic_constraints, nullptr, nullptr))};
}

Error::Code VerifyCertTime(X509* cert, const DateTime& time) {
  ErrorOr<DateTime> not_before = GetNotBeforeTime(cert);
  ErrorOr<DateTime> not_after = GetNotAfterTime(cert);
  if (!not_before || !not_after) {
    return Error::Code::kErrCertsParse;
  }

  if ((time < not_before.value()) || (not_after.value() < time)) {
    return Error::Code::kErrCertsDateInvalid;
  }
  return Error::Code::kNone;
}

bool VerifyPublicKeyLength(EVP_PKEY* public_key) {
  return EVP_PKEY_bits(public_key) >= kMinRsaModulusLengthBits;
}

bssl::UniquePtr<ASN1_BIT_STRING> GetKeyUsage(X509* cert) {
  return bssl::UniquePtr<ASN1_BIT_STRING>{reinterpret_cast<ASN1_BIT_STRING*>(
      X509_get_ext_d2i(cert, NID_key_usage, nullptr, nullptr))};
}

Error::Code VerifyCertificateChain(const std::vector<CertPathStep>& path,
                                   uint32_t step_index,
                                   const DateTime& time) {
  // Default max path length is the number of intermediate certificates.
  int max_pathlen = path.size() - 2;

  std::vector<NameConstraintsPtr> path_name_constraints;
  Error::Code error = Error::Code::kNone;
  uint32_t i = step_index;
  for (; i < path.size() - 1; ++i) {
    X509* subject = path[i + 1].cert;
    X509* issuer = path[i].cert;
    bool is_root = (i == step_index);
    bool issuer_is_self_issued = false;
    if (!is_root) {
      if ((error = VerifyCertTime(issuer, time)) != Error::Code::kNone) {
        return error;
      }
      if (X509_NAME_cmp(X509_get_subject_name(issuer),
                        X509_get_issuer_name(issuer)) != 0) {
        if (max_pathlen == 0) {
          return Error::Code::kErrCertsPathlen;
        }
        --max_pathlen;
      } else {
        issuer_is_self_issued = true;
      }
    } else {
      issuer_is_self_issued = true;
    }

    bssl::UniquePtr<ASN1_BIT_STRING> key_usage = GetKeyUsage(issuer);
    if (key_usage) {
      const int bit =
          ASN1_BIT_STRING_get_bit(key_usage.get(), KeyUsageBits::kKeyCertSign);
      if (bit == 0) {
        return Error::Code::kErrCertsVerifyGeneric;
      }
    }

    // Certificates issued by a valid CA authority shall have the
    // basicConstraints property present with the CA bit set. Self-signed
    // certificates do not have this property present.
    bssl::UniquePtr<BASIC_CONSTRAINTS> basic_constraints =
        GetConstraints(issuer);
    if (!basic_constraints || !basic_constraints->ca) {
      return Error::Code::kErrCertsVerifyGeneric;
    }

    if (basic_constraints->pathlen) {
      if (basic_constraints->pathlen->length != 1) {
        return Error::Code::kErrCertsVerifyGeneric;
      } else {
        const int pathlen = *basic_constraints->pathlen->data;
        if (pathlen < 0) {
          return Error::Code::kErrCertsVerifyGeneric;
        }
        if (pathlen < max_pathlen) {
          max_pathlen = pathlen;
        }
      }
    }

    const X509_ALGOR* issuer_sig_alg;
    X509_get0_signature(nullptr, &issuer_sig_alg, issuer);
    if (X509_ALGOR_cmp(issuer_sig_alg, X509_get0_tbs_sigalg(issuer)) != 0) {
      return Error::Code::kErrCertsVerifyGeneric;
    }

    bssl::UniquePtr<EVP_PKEY> public_key{X509_get_pubkey(issuer)};
    if (!VerifyPublicKeyLength(public_key.get())) {
      return Error::Code::kErrCertsVerifyGeneric;
    }

    // NOTE: (!self-issued || target) -> verify name constraints.  Target case
    // is after the loop.
    if (!issuer_is_self_issued) {
      for (const auto& name_constraints : path_name_constraints) {
        if (NAME_CONSTRAINTS_check(subject, name_constraints.get()) !=
            X509_V_OK) {
          return Error::Code::kErrCertsVerifyGeneric;
        }
      }
    }

    int critical;
    NameConstraintsPtr nc{reinterpret_cast<NAME_CONSTRAINTS*>(
        X509_get_ext_d2i(issuer, NID_name_constraints, &critical, nullptr))};
    if (!nc && critical != -1) {
      // X509_get_ext_d2i's error handling is a little confusing. See
      // https://boringssl.googlesource.com/boringssl/+/215f4a0287/include/openssl/x509.h#1384
      // https://boringssl.googlesource.com/boringssl/+/215f4a0287/include/openssl/x509v3.h#651
      return Error::Code::kErrCertsVerifyGeneric;
    }
    if (nc) {
      path_name_constraints.push_back(std::move(nc));
    }

    // Check that any policy mappings present are _not_ the anyPolicy OID.  Even
    // though we don't otherwise handle policies, this is required by RFC 5280
    // 6.1.4(a).
    //
    // TODO(davidben): Switch to bssl::UniquePtr once
    // https://boringssl-review.googlesource.com/c/boringssl/+/46104 has landed.
    auto* policy_mappings = reinterpret_cast<POLICY_MAPPINGS*>(
        X509_get_ext_d2i(issuer, NID_policy_mappings, nullptr, nullptr));
    if (policy_mappings) {
      const ASN1_OBJECT* any_policy = OBJ_nid2obj(NID_any_policy);
      for (const POLICY_MAPPING* policy_mapping : policy_mappings) {
        const bool either_matches =
            ((OBJ_cmp(policy_mapping->issuerDomainPolicy, any_policy) == 0) ||
             (OBJ_cmp(policy_mapping->subjectDomainPolicy, any_policy) == 0));
        if (either_matches) {
          error = Error::Code::kErrCertsVerifyGeneric;
          break;
        }
      }
      sk_POLICY_MAPPING_free(policy_mappings);
      if (error != Error::Code::kNone) {
        return error;
      }
    }

    // Check that we don't have any unhandled extensions marked as critical.
    int extension_count = X509_get_ext_count(issuer);
    for (int j = 0; j < extension_count; ++j) {
      X509_EXTENSION* extension = X509_get_ext(issuer, j);
      if (X509_EXTENSION_get_critical(extension)) {
        const int nid = OBJ_obj2nid(X509_EXTENSION_get_object(extension));
        if (nid != NID_name_constraints && nid != NID_basic_constraints &&
            nid != NID_key_usage) {
          return Error::Code::kErrCertsVerifyGeneric;
        }
      }
    }

    int nid = X509_get_signature_nid(subject);
    const EVP_MD* digest;
    switch (nid) {
      case NID_sha1WithRSAEncryption:
        digest = EVP_sha1();
        break;
      case NID_sha256WithRSAEncryption:
        digest = EVP_sha256();
        break;
      case NID_sha384WithRSAEncryption:
        digest = EVP_sha384();
        break;
      case NID_sha512WithRSAEncryption:
        digest = EVP_sha512();
        break;
      default:
        return Error::Code::kErrCertsVerifyGeneric;
    }
    uint8_t* tbs = nullptr;
    int tbs_len = i2d_X509_tbs(subject, &tbs);
    if (tbs_len < 0) {
      return Error::Code::kErrCertsVerifyGeneric;
    }
    bssl::UniquePtr<uint8_t> free_tbs{tbs};
    const ASN1_BIT_STRING* signature;
    X509_get0_signature(&signature, nullptr, subject);
    if (!VerifySignedData(
            digest, public_key.get(), {tbs, static_cast<uint32_t>(tbs_len)},
            {ASN1_STRING_get0_data(signature),
             static_cast<uint32_t>(ASN1_STRING_length(signature))})) {
      return Error::Code::kErrCertsVerifyGeneric;
    }
  }
  // NOTE: Other half of ((!self-issued || target) -> check name constraints).
  for (const auto& name_constraints : path_name_constraints) {
    if (NAME_CONSTRAINTS_check(path.back().cert, name_constraints.get()) !=
        X509_V_OK) {
      return Error::Code::kErrCertsVerifyGeneric;
    }
  }
  return error;
}

X509* ParseX509Der(const std::string& der) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(der.data());
  return d2i_X509(nullptr, &data, der.size());
}

}  // namespace

// static
std::unique_ptr<TrustStore> TrustStore::CreateInstanceFromPemFile(
    absl::string_view file_path) {
  std::vector<std::string> der_certs = ReadCertificatesFromPemFile(file_path);
  std::vector<bssl::UniquePtr<X509>> certs;
  certs.reserve(der_certs.size());
  for (const auto& der_cert : der_certs) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(der_cert.data());
    certs.emplace_back(d2i_X509(nullptr, &data, der_cert.size()));
  }
  return std::make_unique<BoringSSLTrustStore>(std::move(certs));
}

// static
std::unique_ptr<TrustStore> TrustStore::CreateInstanceForTest(
    const std::vector<uint8_t>& trust_anchor_der) {
  std::unique_ptr<TrustStore> trust_store =
      std::make_unique<BoringSSLTrustStore>(trust_anchor_der);
  return trust_store;
}

// static
std::unique_ptr<TrustStore> CastTrustStore::Create() {
  std::vector<bssl::UniquePtr<X509>> certs;
  certs.emplace_back(MakeTrustAnchor(kCastRootCaDer));
  certs.emplace_back(MakeTrustAnchor(kEurekaRootCaDer));
  std::unique_ptr<TrustStore> trust_store =
      std::make_unique<BoringSSLTrustStore>(std::move(certs));
  return trust_store;
}

// static
std::unique_ptr<TrustStore> CastCRLTrustStore::Create() {
  std::vector<bssl::UniquePtr<X509>> certs;
  certs.emplace_back(MakeTrustAnchor(kCastCRLRootCaDer));
  std::unique_ptr<TrustStore> trust_store =
      std::make_unique<BoringSSLTrustStore>(std::move(certs));
  return trust_store;
}

BoringSSLTrustStore::BoringSSLTrustStore() {}

BoringSSLTrustStore::BoringSSLTrustStore(
    const std::vector<uint8_t>& trust_anchor_der) {
  certs_.emplace_back(MakeTrustAnchor(trust_anchor_der));
}

BoringSSLTrustStore::BoringSSLTrustStore(
    std::vector<bssl::UniquePtr<X509>> certs)
    : certs_(std::move(certs)) {}

BoringSSLTrustStore::~BoringSSLTrustStore() = default;

ErrorOr<BoringSSLTrustStore::CertificatePathResult>
BoringSSLTrustStore::FindCertificatePath(
    const std::vector<std::string>& der_certs,
    const DateTime& time) {
  if (der_certs.empty()) {
    return Error(Error::Code::kErrCertsMissing, "Missing DER certificates");
  }

  bssl::UniquePtr<X509> target_cert(ParseX509Der(der_certs[0]));
  if (!target_cert) {
    return Error(Error::Code::kErrCertsParse,
                 "FindCertificatePath: Invalid target certificate");
  }
  std::vector<bssl::UniquePtr<X509>> intermediate_certs;
  for (size_t i = 1; i < der_certs.size(); ++i) {
    intermediate_certs.emplace_back(ParseX509Der(der_certs[i]));
    if (!intermediate_certs.back()) {
      return Error(
          Error::Code::kErrCertsParse,
          absl::StrCat(
              "FindCertificatePath: Failed to parse intermediate certificate ",
              i, " of ", der_certs.size()));
    }
  }

  // Basic checks on the target certificate.
  Error::Code valid_time = VerifyCertTime(target_cert.get(), time);
  if (valid_time != Error::Code::kNone) {
    return Error(valid_time,
                 "FindCertificatePath: Failed to verify certificate time");
  }
  bssl::UniquePtr<EVP_PKEY> public_key{X509_get_pubkey(target_cert.get())};
  if (!VerifyPublicKeyLength(public_key.get())) {
    return Error(Error::Code::kErrCertsVerifyGeneric,
                 "FindCertificatePath: Failed with invalid public key length");
  }
  const X509_ALGOR* sig_alg;
  X509_get0_signature(nullptr, &sig_alg, target_cert.get());
  if (X509_ALGOR_cmp(sig_alg, X509_get0_tbs_sigalg(target_cert.get())) != 0) {
    return Error::Code::kErrCertsVerifyGeneric;
  }
  bssl::UniquePtr<ASN1_BIT_STRING> key_usage = GetKeyUsage(target_cert.get());
  if (!key_usage) {
    return Error(Error::Code::kErrCertsRestrictions,
                 "FindCertificatePath: Failed with no key usage");
  }
  int bit =
      ASN1_BIT_STRING_get_bit(key_usage.get(), KeyUsageBits::kDigitalSignature);
  if (bit == 0) {
    return Error(Error::Code::kErrCertsRestrictions,
                 "FindCertificatePath: Failed to get digital signature");
  }

  X509* path_head = target_cert.get();
  std::vector<CertPathStep> path;

  // This vector isn't used as resizable, so instead we allocate the largest
  // possible single path up front.  This would be a single trusted cert, all
  // the intermediate certs used once, and the target cert.
  path.resize(1 + intermediate_certs.size() + 1);

  // Additionally, the path is slightly simpler to deal with if the list is
  // sorted from trust->target, so the path is actually built starting from the
  // end.
  uint32_t first_index = path.size() - 1;
  path[first_index].cert = path_head;

  // Index into |path| of the current frontier of path construction.
  uint32_t path_index = first_index;

  // Whether |path| has reached a certificate in |trust_store| and is ready for
  // verification.
  bool path_cert_in_trust_store = false;

  // Attempt to build a valid certificate chain from |target_cert| to a
  // certificate in |trust_store|.  This loop tries all possible paths in a
  // depth-first-search fashion.  If no valid paths are found, the error
  // returned is whatever the last error was from the last path tried.
  uint32_t trust_store_index = 0;
  uint32_t intermediate_cert_index = 0;
  Error::Code last_error = Error::Code::kNone;
  for (;;) {
    X509_NAME* target_issuer_name = X509_get_issuer_name(path_head);
    OSP_VLOG << "FindCertificatePath: Target certificate issuer name: "
             << X509_NAME_oneline(target_issuer_name, 0, 0);

    // The next issuer certificate to add to the current path.
    X509* next_issuer = nullptr;

    for (uint32_t i = trust_store_index; i < certs_.size(); ++i) {
      X509* trust_store_cert = certs_[i].get();
      X509_NAME* trust_store_cert_name =
          X509_get_subject_name(trust_store_cert);
      OSP_VLOG << "FindCertificatePath: Trust store certificate issuer name: "
               << X509_NAME_oneline(trust_store_cert_name, 0, 0);
      if (X509_NAME_cmp(trust_store_cert_name, target_issuer_name) == 0) {
        CertPathStep& next_step = path[--path_index];
        next_step.cert = trust_store_cert;
        next_step.trust_store_index = i + 1;
        next_step.intermediate_cert_index = 0;
        next_issuer = trust_store_cert;
        path_cert_in_trust_store = true;
        break;
      }
    }
    trust_store_index = 0;
    if (!next_issuer) {
      for (uint32_t i = intermediate_cert_index; i < intermediate_certs.size();
           ++i) {
        X509* intermediate_cert = intermediate_certs[i].get();
        X509_NAME* intermediate_cert_name =
            X509_get_subject_name(intermediate_cert);
        if (X509_NAME_cmp(intermediate_cert_name, target_issuer_name) == 0 &&
            !CertInPath(intermediate_cert_name, path, path_index,
                        first_index)) {
          CertPathStep& next_step = path[--path_index];
          next_step.cert = intermediate_cert;
          next_step.trust_store_index = certs_.size();
          next_step.intermediate_cert_index = i + 1;
          next_issuer = intermediate_cert;
          break;
        }
      }
    }
    intermediate_cert_index = 0;
    if (!next_issuer) {
      if (path_index == first_index) {
        // There are no more paths to try.  Ensure an error is returned.
        if (last_error == Error::Code::kNone) {
          return Error(Error::Code::kErrCertsVerifyUntrustedCert,
                       "FindCertificatePath: Failed after trying all "
                       "certificate paths, no matches");
        }
        return last_error;
      } else {
        CertPathStep& last_step = path[path_index++];
        trust_store_index = last_step.trust_store_index;
        intermediate_cert_index = last_step.intermediate_cert_index;
        continue;
      }
    }

    if (path_cert_in_trust_store) {
      last_error = VerifyCertificateChain(path, path_index, time);
      if (last_error != Error::Code::kNone) {
        CertPathStep& last_step = path[path_index++];
        trust_store_index = last_step.trust_store_index;
        intermediate_cert_index = last_step.intermediate_cert_index;
        path_cert_in_trust_store = false;
      } else {
        break;
      }
    }
    path_head = next_issuer;
  }

  CertificatePathResult result_path;
  result_path.reserve(path.size() - path_index);
  for (uint32_t i = path.size(); i > path_index; --i) {
    result_path.push_back(std::make_unique<BoringSSLParsedCertificate>(
        bssl::UpRef(path[i - 1].cert)));
  }

  OSP_VLOG
      << "FindCertificatePath: Succeeded at validating receiver certificates";
  return result_path;
}

}  // namespace cast
}  // namespace openscreen
