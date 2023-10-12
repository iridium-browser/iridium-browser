// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/test/paths.h"
#include "platform/test/paths_internal.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace {

std::string ReadTestDataPath() {
  std::string exe_path = GetExePath();
  OSP_DCHECK(!exe_path.empty());

  // NOTE: This assumes that the executable is two directories above the source
  // root (e.g. out/Debug/unittests).  This is the standard layout GN expects
  // but is also assumed by Chromium infra.
  int slashes_found = 0;
  int i = exe_path.size() - 1;
  for (; i >= 0; --i) {
    slashes_found += exe_path[i] == '/';
    if (slashes_found == 3) {
      break;
    }
  }
  OSP_DCHECK_EQ(slashes_found, 3);

  return exe_path.substr(0, i + 1) + OPENSCREEN_TEST_DATA_DIR;
}

}  // namespace

const std::string& GetTestDataPath() {
  static std::string data_path = ReadTestDataPath();
  return data_path;
}

}  // namespace openscreen
