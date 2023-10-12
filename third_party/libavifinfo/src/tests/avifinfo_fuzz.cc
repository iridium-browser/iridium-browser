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

static bool Equals(const AvifInfoFeatures& lhs, const AvifInfoFeatures& rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height &&
         lhs.bit_depth == rhs.bit_depth &&
         lhs.num_channels == rhs.num_channels &&
         lhs.has_gainmap == rhs.has_gainmap &&
         lhs.gainmap_item_id == rhs.gainmap_item_id &&
         lhs.primary_item_id_location == rhs.primary_item_id_location &&
         lhs.primary_item_id_bytes == rhs.primary_item_id_bytes;
}

// Test a random bitstream of random size, whether it is valid or not.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t data_size) {
  AvifInfoStatus previous_status_identity = kAvifInfoNotEnoughData;
  AvifInfoStatus previous_status_features = kAvifInfoNotEnoughData;
  AvifInfoFeatures previous_features = {0};

  // Check the consistency of the returned status and features:
  // for a given size and a status that is not kAvifInfoNotEnoughData, any
  // bigger size (of the same data) should return the same status and features.
  for (size_t size = 0; size < data_size; ++size) {
    if (size > 1024 || previous_status_features != kAvifInfoNotEnoughData) {
      // The behavior is unlikely to change: save computing resources.
      size = std::min(data_size, size * 2);
    }

    // Simple raw pointer API.
    AvifInfoFeatures features;
    const AvifInfoStatus status_identity = AvifInfoIdentify(data, size);
    const AvifInfoStatus status_features =
        AvifInfoGetFeatures(data, size, &features);

    // Once a status different than kAvifInfoNotEnoughData is returned, it
    // should not change even with more input bytes.
    if ((previous_status_identity != kAvifInfoNotEnoughData &&
         status_identity != previous_status_identity) ||
        (previous_status_features != kAvifInfoNotEnoughData &&
         status_features != previous_status_features)) {
      std::abort();
    }

    // Check the features.
    if (status_features == previous_status_features) {
      if (!Equals(features, previous_features)) {
        std::abort();
      }
    } else if (status_features == kAvifInfoOk) {
      if (status_identity != kAvifInfoOk) {
        std::abort();
      }
      if (features.width == 0u || features.height == 0u ||
          features.bit_depth == 0u || features.num_channels == 0u ||
          (!features.has_gainmap && features.gainmap_item_id) ||
          !features.primary_item_id_location !=
              !features.primary_item_id_bytes) {
        std::abort();
      }
      if (features.primary_item_id_location &&
          features.primary_item_id_location + features.primary_item_id_bytes >
              size) {
        std::abort();
      }
    } else {
      if (features.width != 0u || features.height != 0u ||
          features.bit_depth != 0u || features.num_channels != 0u ||
          features.has_gainmap != 0u || features.gainmap_item_id != 0u ||
          features.primary_item_id_location != 0u ||
          features.primary_item_id_bytes != 0u) {
        std::abort();
      }
    }

    // Stream API.
    AvifInfoFeatures features_stream;
    StreamData stream_identity = {data, size};
    StreamData stream_features = {data, size};
    const AvifInfoStatus status_identity_stream =
        AvifInfoIdentifyStream(&stream_identity, StreamRead, StreamSkip);
    const AvifInfoStatus status_features_stream = AvifInfoGetFeaturesStream(
        &stream_features, StreamRead, StreamSkip, &features_stream);
    // Both API should have exactly the same behavior, errors included.
    if (status_identity_stream != status_identity) {
      std::abort();
    }
    // AvifInfoGetFeaturesStream() should only be called if
    // AvifInfoIdentifyStream() was a success. Call it anyway to make sure it
    // does not crash, but only check the returned status and features when it
    // makes sense.
    if (status_identity_stream == kAvifInfoOk &&
        (status_features_stream != status_features ||
         !Equals(features_stream, features))) {
      std::abort();
    }

    // Another way of calling the stream API: reuse the stream object that was
    // used for AvifInfoIdentifyStream().
    AvifInfoFeatures features_stream_reused;
    StreamData stream_reused = stream_identity;
    const AvifInfoStatus status_features_reused_stream =
        AvifInfoGetFeaturesStream(&stream_reused, StreamRead, StreamSkip,
                                  &features_stream_reused);
    // AvifInfoGetFeaturesStream() should only be called if
    // AvifInfoIdentifyStream() was a success. Call it anyway to make sure it
    // does not crash, but only check the returned status and features when it
    // makes sense.
    if (status_identity_stream == kAvifInfoOk) {
      if (status_features_reused_stream != status_features_stream) {
        std::abort();
      }
      if (status_features_reused_stream == kAvifInfoOk) {
        // Offset any location-dependent feature as described in avifinfo.h.
        features_stream_reused.primary_item_id_location +=
            size - stream_identity.data_size;
      }
      if (!Equals(features_stream_reused, features_stream)) {
        std::abort();
      }
    }

    // Another way of calling the stream API: no provided user-defined skip
    // method.
    AvifInfoFeatures features_no_skip;
    StreamData stream_identity_no_skip = {data, size};
    StreamData stream_features_no_skip = {data, size};
    const AvifInfoStatus status_identity_no_skip = AvifInfoIdentifyStream(
        &stream_identity_no_skip, StreamRead, /*skip=*/nullptr);
    const AvifInfoStatus status_features_no_skip =
        AvifInfoGetFeaturesStream(&stream_features_no_skip, StreamRead,
                                  /*skip=*/nullptr, &features_no_skip);
    // There may be some difference in status. For example, a valid or invalid
    // status could be returned just after skipping some bytes. If the skip
    // argument is omitted, these bytes will be read instead. If some of these
    // bytes are missing, kAvifInfoNotEnoughData will be returned instead of the
    // expected success or failure status.
    if (status_identity_no_skip != status_identity_stream &&
        !(status_identity_no_skip == kAvifInfoNotEnoughData &&
          status_identity_stream == kAvifInfoOk)) {
      std::abort();
    }
    if (status_identity_stream == kAvifInfoOk) {
      if ((status_features_no_skip != status_features &&
           !(status_features_no_skip == kAvifInfoNotEnoughData &&
             status_features != kAvifInfoOk)) ||
          !Equals(features_no_skip, features)) {
        std::abort();
      }
    }

    previous_status_identity = status_identity;
    previous_status_features = status_features;
    previous_features = features;
  }
  return 0;
}
