// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_target_command_util.h"

#include <algorithm>
#include <sstream>

#include "util/build_config.h"
#include "util/test/test.h"

namespace {

// Helper function that uses a "Writer" to format a list of strings and return
// the generated output as a string.
template <typename Writer, typename Item>
std::string FormatWithWriter(Writer writer, std::vector<Item> items) {
  std::ostringstream out;
  for (const Item& item : items) {
    writer(item, out);
  }
  return out.str();
}

// Helper function running to test "Writer" by formatting N "Items" and
// comparing the resulting string to an expected output.
template <typename Writer, typename Item>
void TestWriter(Writer writer,
                std::string expected,
                std::initializer_list<Item> items) {
  std::string formatted =
      FormatWithWriter(writer, std::vector<Item>(std::move(items)));

  // Manually implement the check and the formatting of the error message to
  // see the difference in the error message (by default the error message
  // would just be "formatted == expected").
  if (formatted != expected) {
    std::ostringstream stream;
    stream << '"' << expected << "\" == \"" << formatted << '"';
    std::string message = stream.str();

    ::testing::TestResult result(false, message.c_str());
    ::testing::AssertHelper(__FILE__, __LINE__, result) = ::testing::Message();
  }
}

}  // anonymous namespace

TEST(NinjaTargetCommandUtil, DefineWriter) {
  TestWriter(DefineWriter(),
// Escaping is different between Windows and Posix.
#if defined(OS_WIN)
             " -DFOO -DBAR=1 \"-DBAZ=\\\"Baz\\\"\"",
#else
             " -DFOO -DBAR=1 -DBAZ=\\\"Baz\\\"",
#endif
             {"FOO", "BAR=1", "BAZ=\"Baz\""});

  TestWriter(DefineWriter(ESCAPE_COMPILATION_DATABASE),
             // Escaping is different between Windows and Posix.
             " -DFOO -DBAR=1 \"-DBAZ=\\\"Baz\\\"\"",
             {"FOO", "BAR=1", "BAZ=\"Baz\""});
}

TEST(NinjaTargetCommandUtil, FrameworkDirsWriter) {
  PathOutput ninja_path_output(SourceDir("//out"), "", ESCAPE_NINJA_COMMAND);
  TestWriter(FrameworkDirsWriter(ninja_path_output, "-F"),
// Escaping is different between Windows and Posix.
#if defined(OS_WIN)
             " -F. \"-FPath$ With$ Spaces\"",
#else
             " -F. -FPath\\$ With\\$ Spaces",
#endif
             {SourceDir("//out"), SourceDir("//out/Path With Spaces")});

  PathOutput space_path_output(SourceDir("//out"), "", ESCAPE_SPACE);
  TestWriter(FrameworkDirsWriter(space_path_output, "-F"),
             " -F. -FPath\\ With\\ Spaces",
             {SourceDir("//out"), SourceDir("//out/Path With Spaces")});
}

TEST(NinjaTargetCommandUtil, FrameworksWriter) {
  TestWriter(FrameworksWriter("-framework "),
// Escaping is different between Windows and Posix.
#if defined(OS_WIN)
             " -framework Foundation -framework \"Name$ With$ Spaces\"",
#else
             " -framework Foundation -framework Name\\$ With\\$ Spaces",
#endif
             {"Foundation.framework", "Name With Spaces.framework"});

  TestWriter(FrameworksWriter(ESCAPE_SPACE, "-framework "),
             " -framework Foundation -framework Name\\ With\\ Spaces",
             {"Foundation.framework", "Name With Spaces.framework"});
}
