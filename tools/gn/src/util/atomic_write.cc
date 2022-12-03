// Copyright (c) 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/atomic_write.h"

#include "base/files/file_util.h"

namespace util {

int WriteFileAtomically(const base::FilePath& filename,
                        const char* data,
                        int size) {
  base::FilePath dir = filename.DirName();
  base::FilePath temp_file_path;

  {
    base::File temp_file =
        base::CreateAndOpenTemporaryFileInDir(dir, &temp_file_path);
    if (!temp_file.IsValid()) {
      return -1;
    }
    if (temp_file.WriteAtCurrentPos(data, size) != size) {
      return -1;
    }
  }

  if (!base::ReplaceFile(temp_file_path, filename, NULL)) {
    return -1;
  }
  return size;
}

}  // namespace util
