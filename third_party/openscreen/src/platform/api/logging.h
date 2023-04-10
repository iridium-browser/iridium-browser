// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_LOGGING_H_
#define PLATFORM_API_LOGGING_H_

#include <sstream>

namespace openscreen {

enum class LogLevel {
  // Very detailed information, often used for evaluating performance or
  // debugging production issues in-the-wild.
  kVerbose = 0,

  // Used occasionally to note events of interest, but not for indicating any
  // problems. This is also used for general console messaging in Open Screen's
  // standalone executables.
  kInfo = 1,

  // Indicates a problem that may or may not lead to an operational failure.
  kWarning = 2,

  // Indicates an operational failure that may or may not cause a component to
  // stop working.
  kError = 3,

  // Indicates a logic flaw, corruption, impossible/unanticipated situation, or
  // operational failure so serious that Open Screen will soon call Break() to
  // abort the current process. Examples: security/privacy risks, memory
  // management issues, API contract violations.
  kFatal = 4,
};

// Returns true if |level| is at or above the level where the embedder will
// record/emit log entries from the code in |file|.
bool IsLoggingOn(LogLevel level, const char* file);

// Record a log entry, consisting of its logging level, location and message.
// The embedder may filter-out entries according to its own policy, but this
// function will not be called if IsLoggingOn(level, file) returns false.
// Whenever |level| is kFatal, Open Screen will call Break() immediately after
// this returns.
//
// |message| is passed as a string stream to avoid unnecessary string copies.
// Embedders can call its rdbuf() or str() methods to access the log message.
void LogWithLevel(LogLevel level,
                  const char* file,
                  int line,
                  std::stringstream message);

// Breaks into the debugger, if one is present. Otherwise, aborts the current
// process (i.e., this function should not return). In production builds, an
// embedder could invoke its infrastructure for performing "dumps," consisting
// of thread stack traces and other relevant process state information, before
// aborting the process.
[[noreturn]] void Break();

}  // namespace openscreen

#endif  // PLATFORM_API_LOGGING_H_
