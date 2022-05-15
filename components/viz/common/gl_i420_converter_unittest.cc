// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gl_i420_converter.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkRect.h"
#include "ui/gfx/geometry/rect.h"

namespace viz {
namespace {

// Syntactic convenience: It's clearer to express the tests in terms of
// left+top+right+bottom coordinates, rather than gfx::Rect's x+y+width+height
// scheme.
SkIRect ToAlignedRect(const SkIRect& rect) {
  const gfx::Rect& result = GLI420Converter::ToAlignedRect(
      gfx::Rect(rect.fLeft, rect.fTop, rect.fRight - rect.fLeft,
                rect.fBottom - rect.fTop));
  return SkIRect{result.x(), result.y(), result.right(), result.bottom()};
}

// Logging convenience.
std::string ToString(const SkIRect& rect) {
  return base::StringPrintf("%d,%d~%d%d", rect.fLeft, rect.fTop, rect.fRight,
                            rect.fBottom);
}

TEST(GLI420ConverterTest, AlignsOutputRects) {
  struct {
    SkIRect expected;
    SkIRect input;
  } kTestCases[] = {
      {SkIRect{0, 0, 0, 0}, SkIRect{0, 0, 0, 0}},

      {SkIRect{-16, 0, 16, 4}, SkIRect{-9, 0, 16, 4}},
      {SkIRect{-8, 0, 16, 4}, SkIRect{-8, 0, 16, 4}},
      {SkIRect{-8, 0, 16, 4}, SkIRect{-7, 0, 16, 4}},
      {SkIRect{-8, 0, 16, 4}, SkIRect{-1, 0, 16, 4}},
      {SkIRect{0, 0, 16, 4}, SkIRect{0, 0, 16, 4}},
      {SkIRect{0, 0, 16, 4}, SkIRect{1, 0, 16, 4}},
      {SkIRect{0, 0, 16, 4}, SkIRect{7, 0, 16, 4}},
      {SkIRect{8, 0, 16, 4}, SkIRect{8, 0, 16, 4}},
      {SkIRect{8, 0, 16, 4}, SkIRect{9, 0, 16, 4}},

      {SkIRect{0, -4, 16, 4}, SkIRect{0, -3, 16, 4}},
      {SkIRect{0, -2, 16, 4}, SkIRect{0, -2, 16, 4}},
      {SkIRect{0, -2, 16, 4}, SkIRect{0, -1, 16, 4}},
      {SkIRect{0, 0, 16, 4}, SkIRect{0, 0, 16, 4}},
      {SkIRect{0, 0, 16, 4}, SkIRect{0, 1, 16, 4}},
      {SkIRect{0, 2, 16, 4}, SkIRect{0, 2, 16, 4}},
      {SkIRect{0, 2, 16, 4}, SkIRect{0, 3, 16, 4}},

      {SkIRect{0, 0, 8, 2}, SkIRect{0, 0, 1, 2}},
      {SkIRect{0, 0, 8, 2}, SkIRect{0, 0, 7, 2}},
      {SkIRect{0, 0, 8, 2}, SkIRect{0, 0, 8, 2}},
      {SkIRect{0, 0, 16, 2}, SkIRect{0, 0, 9, 2}},

      {SkIRect{0, 0, 8, 2}, SkIRect{0, 0, 8, 1}},
      {SkIRect{0, 0, 8, 2}, SkIRect{0, 0, 8, 2}},
      {SkIRect{0, 0, 8, 4}, SkIRect{0, 0, 8, 3}},
      {SkIRect{0, 0, 8, 4}, SkIRect{0, 0, 8, 4}},
  };

  for (const auto& test_case : kTestCases) {
    EXPECT_EQ(test_case.expected, ToAlignedRect(test_case.input))
        << "ToAlignedRect(" << ToString(test_case.input) << ") should be "
        << ToString(test_case.expected);
  }
}

}  // namespace
}  // namespace viz
