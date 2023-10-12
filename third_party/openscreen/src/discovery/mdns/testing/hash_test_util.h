// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_TESTING_HASH_TEST_UTIL_H_
#define DISCOVERY_MDNS_TESTING_HASH_TEST_UTIL_H_

#if HASH_TEST_UTIL_USE_ABSL
#include "discovery/mdns/testing/hash_test_util_abseil.h"
#else
#include "discovery/mdns/testing/hash_test_util_nop.h"
#endif

#endif  // DISCOVERY_MDNS_TESTING_HASH_TEST_UTIL_H_
