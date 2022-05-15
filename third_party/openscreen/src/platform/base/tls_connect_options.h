// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_TLS_CONNECT_OPTIONS_H_
#define PLATFORM_BASE_TLS_CONNECT_OPTIONS_H_

#include "platform/base/macros.h"

namespace openscreen {

struct TlsConnectOptions {
  // This option allows TLS connections to devices without
  // a known hostname, and will typically be “true” for cast code.
  // For example, the cast_socket always sets true.
  bool unsafely_skip_certificate_validation;
};

}  // namespace openscreen

#endif  // PLATFORM_BASE_TLS_CONNECT_OPTIONS_H_
