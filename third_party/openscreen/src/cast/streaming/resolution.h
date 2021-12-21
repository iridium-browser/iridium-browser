// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Resolutions and dimensions (resolutions with a frame rate) are used
// extensively throughout cast streaming. Since their serialization to and
// from JSON is stable and standard, we have a single place definition for
// these for use both in our public APIs and private messages.

#ifndef CAST_STREAMING_RESOLUTION_H_
#define CAST_STREAMING_RESOLUTION_H_

#include "absl/types/optional.h"
#include "json/value.h"
#include "util/simple_fraction.h"

namespace openscreen {
namespace cast {

// A resolution in pixels.
struct Resolution {
  static bool TryParse(const Json::Value& value, Resolution* out);
  bool IsValid() const;
  Json::Value ToJson() const;

  // Returns true if both |width| and |height| of this instance are greater than
  // or equal to that of |other|.
  bool IsSupersetOf(const Resolution& other) const;

  bool operator==(const Resolution& other) const;
  bool operator!=(const Resolution& other) const;

  // Width and height in pixels.
  int width = 0;
  int height = 0;
};

// A resolution in pixels and a frame rate.
struct Dimensions {
  static bool TryParse(const Json::Value& value, Dimensions* out);
  bool IsValid() const;
  Json::Value ToJson() const;

  // Returns true if all properties of this instance are greater than or equal
  // to those of |other|.
  bool IsSupersetOf(const Dimensions& other) const;

  bool operator==(const Dimensions& other) const;
  bool operator!=(const Dimensions& other) const;

  // Get just the width and height fields (for comparisons).
  constexpr Resolution ToResolution() const { return {width, height}; }

  // The effective bit rate is the width * height * frame rate.
  constexpr int effective_bit_rate() const {
    return width * height * static_cast<double>(frame_rate);
  }

  // Width and height in pixels.
  int width = 0;
  int height = 0;

  // |frame_rate| is the maximum maintainable frame rate.
  SimpleFraction frame_rate{0, 1};
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_RESOLUTION_H_
