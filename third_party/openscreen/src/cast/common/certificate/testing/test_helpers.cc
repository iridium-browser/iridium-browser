// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/testing/test_helpers.h"

#include <openssl/bytestring.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdio.h>

#include "absl/strings/match.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace testing {

SignatureTestData::SignatureTestData() = default;

SignatureTestData::~SignatureTestData() {
  OPENSSL_free(message.data());
  OPENSSL_free(sha1.data());
  OPENSSL_free(sha256.data());
}

SignatureTestData ReadSignatureTestData(absl::string_view filename) {
  FILE* fp = fopen(filename.data(), "r");
  OSP_DCHECK(fp);
  SignatureTestData result = {};
  char* name;
  char* header;
  unsigned char* data;
  long length;  // NOLINT
  while (PEM_read(fp, &name, &header, &data, &length) == 1) {
    if (strcmp(name, "MESSAGE") == 0) {
      OSP_DCHECK(result.message.empty());
      result.message = ByteBuffer(data, length);
    } else if (strcmp(name, "SIGNATURE SHA1") == 0) {
      OSP_DCHECK(result.sha1.empty());
      result.sha1 = ByteBuffer(data, length);
    } else if (strcmp(name, "SIGNATURE SHA256") == 0) {
      OSP_DCHECK(result.sha256.empty());
      result.sha256 = ByteBuffer(data, length);
    } else {
      OPENSSL_free(data);
    }
    OPENSSL_free(name);
    OPENSSL_free(header);
  }
  OSP_DCHECK(!result.message.empty());
  OSP_DCHECK(!result.sha1.empty());
  OSP_DCHECK(!result.sha256.empty());

  return result;
}

}  // namespace testing
}  // namespace cast
}  // namespace openscreen
