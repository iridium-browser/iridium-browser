// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Utility and helper functions common to both the agent and browser code.
// This code is not publicly exposed from the SDK.

#ifndef CONTENT_ANALYSIS_COMMON_UTILS_WIN_H_
#define CONTENT_ANALYSIS_COMMON_UTILS_WIN_H_

#include <string>

namespace content_analysis {
namespace sdk {
namespace internal {

// Returns the user SID of the thread or process that calls thie function.
// Returns an empty string on error.
std::string GetUserSID();

// Returns the name of the pipe that should be used to communicate between
// the agent and Google Chrome.  If `sid` is non-empty, make the pip name
// specific to that user.
std::string GetPipeName(const std::string& base, bool user_specific);

}  // internal
}  // namespace sdk
}  // namespace content_analysis

#endif  // CONTENT_ANALYSIS_COMMON_UTILS_WIN_H_