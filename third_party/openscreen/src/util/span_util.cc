// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/span_util.h"

namespace openscreen {

ByteView ByteViewFromString(const std::string& str) {
  return ByteView{reinterpret_cast<const uint8_t* const>(str.data()),
                  str.size()};
}

std::string ByteViewToString(const ByteView& bytes) {
  return std::string(reinterpret_cast<const char* const>(bytes.data()),
                     bytes.size());
}

}  // namespace openscreen
