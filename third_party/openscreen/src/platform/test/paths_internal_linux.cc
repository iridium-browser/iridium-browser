// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits.h>
#include <unistd.h>

#include "platform/test/paths_internal.h"
#include "util/std_util.h"

namespace openscreen {

std::string GetExePath() {
  std::string path(_POSIX_PATH_MAX, 0);
  int ret = readlink("/proc/self/exe", data(path), path.size());
  if (ret < 0) {
    path.resize(0);
  } else {
    path.resize(ret);
  }
  return path;
}

}  // namespace openscreen
