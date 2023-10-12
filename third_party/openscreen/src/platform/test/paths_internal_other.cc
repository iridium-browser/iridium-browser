// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/test/paths_internal.h"
#include "util/osp_logging.h"

namespace openscreen {

// NOTE: This is only for linking purposes in Chromium builds.
std::string GetExePath() {
  OSP_NOTREACHED();
}

}  // namespace openscreen
