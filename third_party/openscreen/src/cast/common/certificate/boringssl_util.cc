// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/boringssl_util.h"

#include <cstdint>

#include "absl/strings/string_view.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

// Parse the data in |time| at |index| as a two-digit ascii number. Note this
// function assumes the caller already did a bounds check and checked the inputs
// are digits.
uint8_t ParseAsn1TimeDoubleDigit(absl::string_view time, size_t index) {
  OSP_DCHECK_LT(index + 1, time.size());
  OSP_DCHECK('0' <= time[index] && time[index] <= '9');
  OSP_DCHECK('0' <= time[index + 1] && time[index + 1] <= '9');
  return (time[index] - '0') * 10 + (time[index + 1] - '0');
}

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

}  // namespace

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

ErrorOr<DateTime> GetNotBeforeTime(X509* cert) {
  bssl::UniquePtr<ASN1_GENERALIZEDTIME> not_before_asn1{
      ASN1_TIME_to_generalizedtime(X509_get0_notBefore(cert), nullptr)};
  if (!not_before_asn1) {
    return Error(Error::Code::kErrCertsParse,
                 "Failed to retrieve cert notBefore");
  }
  DateTime result;
  if (!ParseAsn1GeneralizedTime(not_before_asn1.get(), &result)) {
    return Error(Error::Code::kErrCertsParse, "Failed to parse cert notBefore");
  }
  return result;
}

ErrorOr<DateTime> GetNotAfterTime(X509* cert) {
  bssl::UniquePtr<ASN1_GENERALIZEDTIME> not_after_asn1{
      ASN1_TIME_to_generalizedtime(X509_get0_notAfter(cert), nullptr)};
  if (!not_after_asn1) {
    return Error(Error::Code::kErrCertsParse,
                 "Failed to retrieve parse cert notAfter");
  }
  DateTime result;
  if (!ParseAsn1GeneralizedTime(not_after_asn1.get(), &result)) {
    return Error(Error::Code::kErrCertsParse, "Failed to parse cert notAfter");
  }
  return result;
}

}  // namespace cast
}  // namespace openscreen
