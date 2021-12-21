// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_CAST_TRUST_STORE_H_
#define CAST_COMMON_CERTIFICATE_CAST_TRUST_STORE_H_

#include <vector>

#include "absl/strings/string_view.h"
#include "cast/common/certificate/cast_cert_validator_internal.h"

namespace openscreen {
namespace cast {

class CastTrustStore {
 public:
  static CastTrustStore* GetInstance();
  static void ResetInstance();

  static CastTrustStore* CreateInstanceForTest(
      const std::vector<uint8_t>& trust_anchor_der);

  static CastTrustStore* CreateInstanceFromPemFile(absl::string_view file_path);

  CastTrustStore();
  explicit CastTrustStore(const std::vector<uint8_t>& trust_anchor_der);
  explicit CastTrustStore(TrustStore trust_store);
  CastTrustStore(const CastTrustStore&) = delete;
  ~CastTrustStore();
  CastTrustStore& operator=(const CastTrustStore&) = delete;

  TrustStore* trust_store() { return &trust_store_; }

 private:
  static CastTrustStore* store_;
  TrustStore trust_store_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_CAST_TRUST_STORE_H_
