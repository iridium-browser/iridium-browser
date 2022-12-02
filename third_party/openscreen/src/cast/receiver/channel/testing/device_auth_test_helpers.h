// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_RECEIVER_CHANNEL_TESTING_DEVICE_AUTH_TEST_HELPERS_H_
#define CAST_RECEIVER_CHANNEL_TESTING_DEVICE_AUTH_TEST_HELPERS_H_

#include <memory>
#include <vector>

#include "absl/strings/string_view.h"
#include "cast/receiver/channel/device_auth_namespace_handler.h"
#include "cast/receiver/channel/static_credentials.h"

namespace openscreen {
namespace cast {

class ParsedCertificate;
class TrustStore;

void InitStaticCredentialsFromFiles(
    StaticCredentialsProvider* creds,
    std::unique_ptr<ParsedCertificate>* parsed_cert,
    std::unique_ptr<TrustStore>* fake_trust_store,
    absl::string_view privkey_filename,
    absl::string_view chain_filename,
    absl::string_view tls_filename);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_RECEIVER_CHANNEL_TESTING_DEVICE_AUTH_TEST_HELPERS_H_
