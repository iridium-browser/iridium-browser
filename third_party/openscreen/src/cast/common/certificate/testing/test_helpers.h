// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_TESTING_TEST_HELPERS_H_
#define CAST_COMMON_CERTIFICATE_TESTING_TEST_HELPERS_H_

#include "absl/strings/string_view.h"
#include "platform/base/span.h"

namespace openscreen {
namespace cast {
namespace testing {

class SignatureTestData {
 public:
  SignatureTestData();
  ~SignatureTestData();

  ByteBuffer message;
  ByteBuffer sha1;
  ByteBuffer sha256;
};

SignatureTestData ReadSignatureTestData(absl::string_view filename);

}  // namespace testing
}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_TESTING_TEST_HELPERS_H_
