// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/frameworks_utils.h"

#include "gn/filesystem_utils.h"

namespace {

// Name of the extension of frameworks.
const char kFrameworkExtension[] = "framework";

}  // anonymous namespace

std::string_view GetFrameworkName(const std::string& file) {
  if (FindFilenameOffset(file) != 0)
    return std::string_view();

  std::string_view extension = FindExtension(&file);
  if (extension != kFrameworkExtension)
    return std::string_view();

  return FindFilenameNoExtension(&file);
}
