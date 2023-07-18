// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/common/config.h"
#include "discovery/mdns/mdns_reader.h"

namespace openscreen {
namespace discovery {
void Fuzz(const uint8_t* data, size_t size) {
  MdnsReader reader(Config{}, data, size);
  reader.Read();
}
}  // namespace discovery
}  // namespace openscreen
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  openscreen::discovery::Fuzz(data, size);
  return 0;
}
