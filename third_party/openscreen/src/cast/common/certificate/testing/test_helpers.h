// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_TESTING_TEST_HELPERS_H_
#define CAST_COMMON_CERTIFICATE_TESTING_TEST_HELPERS_H_

#include <openssl/evp.h>

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "cast/common/public/certificate_types.h"

namespace openscreen {
namespace cast {
namespace testing {

class SignatureTestData {
 public:
  SignatureTestData();
  ~SignatureTestData();

  ConstDataSpan message;
  ConstDataSpan sha1;
  ConstDataSpan sha256;
};

SignatureTestData ReadSignatureTestData(absl::string_view filename);

}  // namespace testing
}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_TESTING_TEST_HELPERS_H_
