// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/cast_cert_validator_internal.h"

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
#include <vector>

#include "absl/strings/str_cat.h"
#include "cast/common/certificate/types.h"
#include "util/crypto/pem_helpers.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

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

// Parse the data in |time| at |index| as a two-digit ascii number. Note this
// function assumes the caller already did a bounds check and checked the inputs
// are digits.
uint8_t ParseAsn1TimeDoubleDigit(absl::string_view time, size_t index) {
  OSP_DCHECK_LT(index + 1, time.size());
  OSP_DCHECK('0' <= time[index] && time[index] <= '9');
  OSP_DCHECK('0' <= time[index + 1] && time[index + 1] <= '9');
  return (time[index] - '0') * 10 + (time[index + 1] - '0');
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
  DateTime not_before;
  DateTime not_after;
  if (!GetCertValidTimeRange(cert, &not_before, &not_after)) {
    return Error::Code::kErrCertsVerifyGeneric;
  }

  if ((time < not_before) || (not_after < time)) {
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

// Parses DateTime with additional restrictions laid out by RFC 5280
// 4.1.2.5.2.
bool ParseAsn1GeneralizedTime(ASN1_GENERALIZEDTIME* time, DateTime* out) {
  static constexpr uint8_t kDaysPerMonth[] = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
  };

  absl::string_view time_str{
      reinterpret_cast<const char*>(ASN1_STRING_get0_data(time)),
      static_cast<size_t>(ASN1_STRING_length(time))};
  if (time_str.size() != 15) {
    return false;
  }
  if (time_str[14] != 'Z') {
    return false;
  }
  for (size_t i = 0; i < 14; ++i) {
    if (time_str[i] < '0' || time_str[i] > '9') {
      return false;
    }
  }
  out->year = ParseAsn1TimeDoubleDigit(time_str, 0) * 100 +
              ParseAsn1TimeDoubleDigit(time_str, 2);
  out->month = ParseAsn1TimeDoubleDigit(time_str, 4);
  out->day = ParseAsn1TimeDoubleDigit(time_str, 6);
  out->hour = ParseAsn1TimeDoubleDigit(time_str, 8);
  out->minute = ParseAsn1TimeDoubleDigit(time_str, 10);
  out->second = ParseAsn1TimeDoubleDigit(time_str, 12);
  if (out->month == 0 || out->month > 12) {
    return false;
  }
  int days_per_month = kDaysPerMonth[out->month - 1];
  if (out->month == 2) {
    if (out->year % 4 == 0 && (out->year % 100 != 0 || out->year % 400 == 0)) {
      days_per_month = 29;
    } else {
      days_per_month = 28;
    }
  }
  if (out->day == 0 || out->day > days_per_month) {
    return false;
  }
  if (out->hour > 23) {
    return false;
  }
  if (out->minute > 59) {
    return false;
  }
  // Leap seconds are allowed.
  if (out->second > 60) {
    return false;
  }
  return true;
}

bool GetCertValidTimeRange(X509* cert,
                           DateTime* not_before,
                           DateTime* not_after) {
  bssl::UniquePtr<ASN1_GENERALIZEDTIME> not_before_asn1{
      ASN1_TIME_to_generalizedtime(X509_get0_notBefore(cert), nullptr)};
  bssl::UniquePtr<ASN1_GENERALIZEDTIME> not_after_asn1{
      ASN1_TIME_to_generalizedtime(X509_get0_notAfter(cert), nullptr)};
  if (!not_before_asn1 || !not_after_asn1) {
    return false;
  }
  return ParseAsn1GeneralizedTime(not_before_asn1.get(), not_before) &&
         ParseAsn1GeneralizedTime(not_after_asn1.get(), not_after);
}

// static
TrustStore TrustStore::CreateInstanceFromPemFile(absl::string_view file_path) {
  TrustStore store;

  std::vector<std::string> certs = ReadCertificatesFromPemFile(file_path);
  for (const auto& der_cert : certs) {
    const uint8_t* data = (const uint8_t*)der_cert.data();
    store.certs.emplace_back(d2i_X509(nullptr, &data, der_cert.size()));
  }

  return store;
}

bool VerifySignedData(const EVP_MD* digest,
                      EVP_PKEY* public_key,
                      const ConstDataSpan& data,
                      const ConstDataSpan& signature) {
  // This code assumes the signature algorithm was RSASSA PKCS#1 v1.5 with
  // |digest|.
  bssl::ScopedEVP_MD_CTX ctx;
  if (!EVP_DigestVerifyInit(ctx.get(), nullptr, digest, nullptr, public_key)) {
    return false;
  }
  return (EVP_DigestVerify(ctx.get(), signature.data, signature.length,
                           data.data, data.length) == 1);
}

Error FindCertificatePath(const std::vector<std::string>& der_certs,
                          const DateTime& time,
                          CertificatePathResult* result_path,
                          TrustStore* trust_store) {
  if (der_certs.empty()) {
    return Error(Error::Code::kErrCertsMissing, "Missing DER certificates");
  }

  bssl::UniquePtr<X509>& target_cert = result_path->target_cert;
  std::vector<bssl::UniquePtr<X509>>& intermediate_certs =
      result_path->intermediate_certs;
  target_cert.reset(ParseX509Der(der_certs[0]));
  if (!target_cert) {
    return Error(Error::Code::kErrCertsParse,
                 "FindCertificatePath: Invalid target certificate");
  }
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

    for (uint32_t i = trust_store_index; i < trust_store->certs.size(); ++i) {
      X509* trust_store_cert = trust_store->certs[i].get();
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
          next_step.trust_store_index = trust_store->certs.size();
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

  result_path->path.reserve(path.size() - path_index);
  for (uint32_t i = path_index; i < path.size(); ++i) {
    result_path->path.push_back(path[i].cert);
  }

  OSP_VLOG
      << "FindCertificatePath: Succeeded at validating receiver certificates";
  return Error::Code::kNone;
}

}  // namespace cast
}  // namespace openscreen
