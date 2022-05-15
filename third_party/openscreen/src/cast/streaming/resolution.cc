// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/resolution.h"

#include <utility>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "cast/streaming/message_fields.h"
#include "platform/base/error.h"
#include "util/json/json_helpers.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

namespace {

/// Dimension properties.
// Width in pixels.
static constexpr char kWidth[] = "width";

// Height in pixels.
static constexpr char kHeight[] = "height";

// Frame rate as a rational decimal number or fraction.
// E.g. 30 and "3000/1001" are both valid representations.
static constexpr char kFrameRate[] = "frameRate";

// Choice of epsilon for double comparison allows for proper comparison
// for both aspect ratios and frame rates. For frame rates, it is based on the
// broadcast rate of 29.97fps, which is actually 29.976. For aspect ratios, it
// allows for a one-pixel difference at a 4K resolution, we want it to be
// relatively high to avoid false negative comparison results.
bool FrameRateEquals(double a, double b) {
  const double kEpsilonForFrameRateComparisons = .0001;
  return std::abs(a - b) < kEpsilonForFrameRateComparisons;
}

}  // namespace

bool Resolution::TryParse(const Json::Value& root, Resolution* out) {
  if (!json::TryParseInt(root[kWidth], &(out->width)) ||
      !json::TryParseInt(root[kHeight], &(out->height))) {
    return false;
  }
  return out->IsValid();
}

bool Resolution::IsValid() const {
  return width > 0 && height > 0;
}

Json::Value Resolution::ToJson() const {
  OSP_DCHECK(IsValid());
  Json::Value root;
  root[kWidth] = width;
  root[kHeight] = height;

  return root;
}

bool Resolution::operator==(const Resolution& other) const {
  return std::tie(width, height) == std::tie(other.width, other.height);
}

bool Resolution::operator!=(const Resolution& other) const {
  return !(*this == other);
}

bool Resolution::IsSupersetOf(const Resolution& other) const {
  return width >= other.width && height >= other.height;
}

bool Dimensions::TryParse(const Json::Value& root, Dimensions* out) {
  if (!json::TryParseInt(root[kWidth], &(out->width)) ||
      !json::TryParseInt(root[kHeight], &(out->height)) ||
      !(root[kFrameRate].isNull() ||
        json::TryParseSimpleFraction(root[kFrameRate], &(out->frame_rate)))) {
    return false;
  }
  return out->IsValid();
}

bool Dimensions::IsValid() const {
  return width > 0 && height > 0 && frame_rate.is_positive();
}

Json::Value Dimensions::ToJson() const {
  OSP_DCHECK(IsValid());
  Json::Value root;
  root[kWidth] = width;
  root[kHeight] = height;
  root[kFrameRate] = frame_rate.ToString();

  return root;
}

bool Dimensions::operator==(const Dimensions& other) const {
  return (std::tie(width, height) == std::tie(other.width, other.height) &&
          FrameRateEquals(static_cast<double>(frame_rate),
                          static_cast<double>(other.frame_rate)));
}

bool Dimensions::operator!=(const Dimensions& other) const {
  return !(*this == other);
}

bool Dimensions::IsSupersetOf(const Dimensions& other) const {
  if (static_cast<double>(frame_rate) !=
      static_cast<double>(other.frame_rate)) {
    return static_cast<double>(frame_rate) >=
           static_cast<double>(other.frame_rate);
  }

  return ToResolution().IsSupersetOf(other.ToResolution());
}

}  // namespace cast
}  // namespace openscreen
