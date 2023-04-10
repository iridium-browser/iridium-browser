// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_UTIL_READ_FILE_H_
#define TESTING_UTIL_READ_FILE_H_

#include <string>

#include "absl/strings/string_view.h"

namespace openscreen {

std::string ReadEntireFileToString(absl::string_view filename);

}  // namespace openscreen

#endif  // TESTING_UTIL_READ_FILE_H_
