// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include <cstdint>

#include "cast/common/channel/message_framer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  openscreen::cast::message_serialization::TryDeserialize(
      absl::Span<const uint8_t>(data, size));
  return 0;
}
