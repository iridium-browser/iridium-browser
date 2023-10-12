// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_LOGGING_TEST_H_
#define PLATFORM_IMPL_LOGGING_TEST_H_

#include <string>
#include <vector>

// These functions should only be called from logging unittests.

namespace openscreen {

// Append logging output to |messages|.  Each log entry will be written as a new
// element including a newline.  Pass nullptr to stop appending output.  Calling
// this does not affect the behavior of SetLogFifoOrDie().  Normally this should
// only be called for tests as it creates an append-only buffer of log messages
// in memory.
void SetLogBufferForTest(std::vector<std::string>* messages);

}  // namespace openscreen

#endif  // PLATFORM_IMPL_LOGGING_TEST_H_
