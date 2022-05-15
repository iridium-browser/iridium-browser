// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_TESTING_HASH_TEST_UTIL_NOP_H_
#define DISCOVERY_MDNS_TESTING_HASH_TEST_UTIL_NOP_H_

#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

template <int&..., typename T>
testing::AssertionResult VerifyTypeImplementsAbslHashCorrectly(
    std::initializer_list<T> values) {
  return testing::AssertionSuccess();
}

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_TESTING_HASH_TEST_UTIL_NOP_H_
