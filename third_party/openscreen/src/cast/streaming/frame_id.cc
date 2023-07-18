// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/frame_id.h"

namespace openscreen {
namespace cast {

std::ostream& operator<<(std::ostream& out, const FrameId rhs) {
  return out << rhs.ToString();
}

std::string FrameId::ToString() const {
  if (is_null())
    return "F<null>";

  return "F" + std::to_string(value());
}

}  // namespace cast
}  // namespace openscreen
