// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits.h>
#include <mach-o/dyld.h>
#include <stdlib.h>

#include "platform/test/paths_internal.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {

std::string GetExePath() {
  uint32_t path_size = 0;
  _NSGetExecutablePath(nullptr, &path_size);
  OSP_DCHECK(path_size > 0u);
  std::string exe_path(path_size, 0);
  int ret = _NSGetExecutablePath(data(exe_path), &path_size);
  OSP_DCHECK_EQ(ret, 0);
  char* resolved = realpath(exe_path.c_str(), nullptr);
  std::string final_path(resolved);
  free(resolved);
  return final_path;
}

}  // namespace openscreen
