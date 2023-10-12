// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>

#include <iostream>
#include <string>

#include "gtest/gtest.h"

// The test main must toggle logging and trace logging features because
// tests will be run in environments that support them (in which case we
// want as much debugging information as possible), and environments that
// don't, so they must be disabled. If ENABLE_PLATFORM_IMPL and
// ENABLE_TRACE_LOGGING are both false, then we should no Open Screen
// dependencies.
#ifdef ENABLE_PLATFORM_IMPL
#include "platform/impl/logging.h"
#endif
#ifdef ENABLE_TRACE_LOGGING
#include "platform/impl/text_trace_logging_platform.h"
#endif

namespace {
void LogUsage(const char* argv0) {
  std::cerr << R"(
usage: )" << argv0
            << R"( <options>

options:
  -t, --tracing: Enable performance tracing logging.

  -v, --verbose: Enable verbose logging.

  -h, --help: Show this help message.
)";
}

struct GlobalTestState {
#ifdef ENABLE_TRACE_LOGGING
  std::unique_ptr<openscreen::TextTraceLoggingPlatform> trace_logger;
#endif
  bool args_are_valid = false;
};

GlobalTestState InitFromArgs(int argc, char** argv) {
  // A note about modifying command line arguments: consider uniformity
  // between all Open Screen executables. If it is a platform feature
  // being exposed, consider if it applies to the standalone receiver,
  // standalone sender, osp demo, and test_main argument options.
  const struct option kArgumentOptions[] = {
      {"tracing", no_argument, nullptr, 't'},
      {"verbose", no_argument, nullptr, 'v'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0}};

  GlobalTestState state;
  int ch = -1;
  while ((ch = getopt_long(argc, argv, "tvh", kArgumentOptions, nullptr)) !=
         -1) {
    switch (ch) {
#ifdef ENABLE_TRACE_LOGGING
      case 't':
        state.trace_logger =
            std::make_unique<openscreen::TextTraceLoggingPlatform>();
        break;
#endif
// When not built with Chrome, log level default is warning. When we are built
// with Chrome, we have no way of knowing or setting log level.
#ifdef ENABLE_PLATFORM_IMPL
      case 'v':
        openscreen::SetLogLevel(openscreen::LogLevel::kVerbose);
        break;
#endif
      case 'h':
        LogUsage(argv[0]);
        return state;
    }
  }

  state.args_are_valid = true;
  return state;
}
}  // namespace

// Googletest strongly recommends that we roll our own main
// function if we want to do global test environment setup.
// See the below link for more info;
// https://github.com/google/googletest/blob/master/docs/advanced.md#sharing-resources-between-tests-in-the-same-test-suite
// TODO(issuetracker.google.com/172242670): rename reference to "main"
// once googletest has a "main" branch.
// This main method is a drop-in replacement for anywhere that currently
// depends on gtest_main, meaning it can be linked into any test-only binary
// to provide a main implementation that supports setting flags and other
// state that we want shared between all tests.
int main(int argc, char** argv) {
  auto state = InitFromArgs(argc, argv);
  if (!state.args_are_valid) {
    return 1;
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
