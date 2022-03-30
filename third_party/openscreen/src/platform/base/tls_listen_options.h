// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_TLS_LISTEN_OPTIONS_H_
#define PLATFORM_BASE_TLS_LISTEN_OPTIONS_H_

#include <cstdint>

#include "platform/base/macros.h"

namespace openscreen {

struct TlsListenOptions {
  uint32_t backlog_size;
};

}  // namespace openscreen

#endif  // PLATFORM_BASE_TLS_LISTEN_OPTIONS_H_
