// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/attribute.h"
#include "libipp/frame.h"
#include "libipp/parser.h"

#include <cstdint>
#include <vector>

void BrowseCollection(ipp::Collection& coll) {
  for (ipp::Attribute& attr : coll) {
    if (attr.Tag() == ipp::ValueTag::collection) {
      const size_t n = attr.Size();
      for (size_t i = 0; i < n; ++i) {
        BrowseCollection(attr.Colls()[i]);
      }
    }
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Parse the input data.
  ipp::SimpleParserLog log;
  ipp::Frame frame = ipp::Parse(data, size, log);
  // Browse the obtained frame.
  for (ipp::GroupTag gt : ipp::kGroupTags) {
    ipp::CollsView colls = frame.Groups(gt);
    for (ipp::Collection& coll : colls) {
      BrowseCollection(coll);
    }
  }

  return 0;
}
