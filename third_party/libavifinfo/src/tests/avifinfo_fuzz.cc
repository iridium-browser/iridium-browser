// Copyright (c) 2021, Alliance for Open Media. All rights reserved
//
// This source code is subject to the terms of the BSD 2 Clause License and
// the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
// was not distributed with this source code in the LICENSE file, you can
// obtain it at www.aomedia.org/license/software. If the Alliance for Open
// Media Patent License 1.0 was not distributed with this source code in the
// PATENTS file, you can obtain it at www.aomedia.org/license/patent.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "avifinfo.h"

//------------------------------------------------------------------------------
// Stream definition.

struct StreamData {
  const uint8_t* data;
  size_t data_size;
};

static const uint8_t* StreamRead(void* stream, size_t num_bytes) {
  if (stream == nullptr) std::abort();
  if (num_bytes < 1 || num_bytes > AVIFINFO_MAX_NUM_READ_BYTES) std::abort();

  StreamData* stream_data = reinterpret_cast<StreamData*>(stream);
  if (num_bytes > stream_data->data_size) return nullptr;
  const uint8_t* data = stream_data->data;
  stream_data->data += num_bytes;
  stream_data->data_size -= num_bytes;
  return data;
}

static void StreamSkip(void* stream, size_t num_bytes) {
  if (stream == nullptr) std::abort();
  if (num_bytes < 1) std::abort();

  StreamData* stream_data = reinterpret_cast<StreamData*>(stream);
  num_bytes = std::min(num_bytes, stream_data->data_size);
  stream_data->data += num_bytes;
  stream_data->data_size -= num_bytes;
}

//------------------------------------------------------------------------------

// Test a random bitstream of random size, whether it is valid or not.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t data_size) {
  AvifInfoStatus previous_status_identity = kAvifInfoNotEnoughData;
  AvifInfoStatus previous_status_features = kAvifInfoNotEnoughData;
  AvifInfoFeatures previous_features = {0};

  // Check the consistency of the returned status and features:
  // for a given size and a status that is not kAvifInfoNotEnoughData, any
  // bigger size (of the same data) should return the same status and features.
  for (size_t size = 0; size < data_size; ++size) {
    // Speed up considerably once it is highly likely the header is parsed.
    if (size > 4096) size = std::min(data_size, size + 511);

    StreamData stream = {data, size};
    AvifInfoFeatures features;
    const AvifInfoStatus status_identity =
        AvifInfoIdentifyStream(&stream, StreamRead, StreamSkip);
    const AvifInfoStatus status_features =
        AvifInfoGetFeaturesStream(&stream, StreamRead, StreamSkip, &features);

    if ((previous_status_identity != kAvifInfoNotEnoughData &&
         status_identity != previous_status_identity) ||
        (previous_status_features != kAvifInfoNotEnoughData &&
         status_features != previous_status_features)) {
      std::abort();
    }

    if (status_features == previous_status_features) {
      if (features.width != previous_features.width ||
          features.height != previous_features.height ||
          features.bit_depth != previous_features.bit_depth ||
          features.num_channels != previous_features.num_channels) {
        std::abort();
      }
    } else if (status_features == kAvifInfoOk) {
      if (features.width == 0u || features.height == 0u ||
          features.bit_depth == 0u || features.num_channels == 0u) {
        std::abort();
      }
    } else {
      if (features.width != 0u || features.height != 0u ||
          features.bit_depth != 0u || features.num_channels != 0u) {
        std::abort();
      }
    }

    previous_status_identity = status_identity;
    previous_status_features = status_features;
    previous_features = features;
  }
  return 0;
}
