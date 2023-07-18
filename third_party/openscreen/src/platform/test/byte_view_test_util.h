// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_TEST_BYTE_VIEW_TEST_UTIL_H_
#define PLATFORM_TEST_BYTE_VIEW_TEST_UTIL_H_

#include "platform/base/span.h"

namespace openscreen {

// Asserts that `first` and `second` have the same non-zero length, and are
// views over the same bytes.  (Not that they are the pointers to the same
// memory.)
void ExpectByteViewsHaveSameBytes(const ByteView& first,
                                  const ByteView& second);

// Asserts that `first` and `second` have the same non-zero length, but are
// views over different bytes.  (Not that they are the pointers to different
// memory.)
void ExpectByteViewsHaveDifferentBytes(const ByteView& first,
                                       const ByteView& second);

// Returns a ByteView from the null-terminated character literal `literal`.
ByteView ByteViewFromLiteral(const char* literal);

}  // namespace openscreen

#endif  // PLATFORM_TEST_BYTE_VIEW_TEST_UTIL_H_
