// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/util/read_file.h"

#include <stdio.h>

namespace openscreen {

std::string ReadEntireFileToString(absl::string_view filename) {
  FILE* file = fopen(filename.data(), "r");
  if (file == nullptr) {
    return {};
  }
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  std::string contents(file_size, 0);
  int bytes_read = 0;
  while (bytes_read < file_size) {
    size_t ret = fread(&contents[bytes_read], 1, file_size - bytes_read, file);
    if (ret == 0 && ferror(file)) {
      return {};
    } else {
      bytes_read += ret;
    }
  }
  fclose(file);

  return contents;
}

}  // namespace openscreen
