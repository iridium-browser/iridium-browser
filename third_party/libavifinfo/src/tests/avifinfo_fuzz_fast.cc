// Copyright (c) 2023, Alliance for Open Media. All rights reserved
//
// This source code is subject to the terms of the BSD 2 Clause License and
// the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
// was not distributed with this source code in the LICENSE file, you can
// obtain it at www.aomedia.org/license/software. If the Alliance for Open
// Media Patent License 1.0 was not distributed with this source code in the
// PATENTS file, you can obtain it at www.aomedia.org/license/patent.

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "avifinfo.h"

//------------------------------------------------------------------------------

// Test a random bitstream of random size, whether it is valid or not.
// Let the fuzzer exercise any input as fast as possible, to expand as much
// coverage as possible for a given corpus; hence the simple API use and the
// limited correctness verifications.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t data_size) {
  const AvifInfoStatus status_identity = AvifInfoIdentify(data, data_size);

  AvifInfoFeatures features;
  const AvifInfoStatus status_features =
      AvifInfoGetFeatures(data, data_size, &features);

  if (status_features == kAvifInfoOk) {
    if (status_identity != kAvifInfoOk) {
      std::abort();
    }
    if (features.width == 0u || features.height == 0u ||
        features.bit_depth == 0u || features.num_channels == 0u ||
        (!features.has_gainmap && features.gainmap_item_id) ||
        !features.primary_item_id_location != !features.primary_item_id_bytes) {
      std::abort();
    }
    if (features.primary_item_id_location &&
        features.primary_item_id_location + features.primary_item_id_bytes >
            data_size) {
      std::abort();
    }
  }
  return 0;
}
