// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_LOGGING_H_
#define PLATFORM_IMPL_LOGGING_H_

#include <string>

#include "util/osp_logging.h"

namespace openscreen {

// Append logging output to a named FIFO having the given |filename|. If the
// file does not exist, an attempt is made to auto-create it. If unsuccessful,
// abort the program. If this is never called, logging continues to output to
// the default destination (stderr).
void SetLogFifoOrDie(const char* filename);

// Set the global logging level. If this is never called, kWarning is the
// default.
void SetLogLevel(LogLevel level);

// Returns the current global logging level.
LogLevel GetLogLevel();

// Log a trace message. Used by the text trace logging platform.
void LogTraceMessage(const std::string& message);

}  // namespace openscreen

#endif  // PLATFORM_IMPL_LOGGING_H_
