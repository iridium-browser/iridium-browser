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

// The default size of the buffer used to hold messages received from
// Google Chrome.
const DWORD kBufferSize = 4096;

// Returns the user SID of the thread or process that calls thie function.
// Returns an empty string on error.
std::string GetUserSID();

// Returns the name of the pipe that should be used to communicate between
// the agent and Google Chrome.  If `sid` is non-empty, make the pip name
// specific to that user.
std::string GetPipeName(const std::string& base, bool user_specific);

// Creates a named pipe with the give name.  If `is_first_pipe` is true,
// fail if this is not the first pipe using this name.
//
// This function create a pipe whose DACL allow full control to the creator
// owner and administrators.  If `user_specific` the DACL only allows the
// logged on user to read from and write to the pipe.  Otherwise anyone logged
// in can read from and write to the pipe.
//
// A handle to the pipe is retuned in `handle`.
DWORD CreatePipe(const std::string& name,
                 bool user_specific,
                 bool is_first_pipe,
                 HANDLE* handle);

}  // internal
}  // namespace sdk
}  // namespace content_analysis

#endif  // CONTENT_ANALYSIS_COMMON_UTILS_WIN_H_