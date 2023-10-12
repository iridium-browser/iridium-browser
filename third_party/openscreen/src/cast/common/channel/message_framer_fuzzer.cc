// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include <cstdint>

#include "cast/common/channel/message_framer.h"
#include "platform/base/span.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  openscreen::cast::message_serialization::TryDeserialize(
      openscreen::ByteView(data, size));
  return 0;
}
