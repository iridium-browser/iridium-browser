// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/escape.h"
#include "gn/string_output_buffer.h"
#include "util/test/test.h"

TEST(Escape, Ninja) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA;
  std::string result = EscapeString("asdf: \"$\\bar", opts, nullptr);
  EXPECT_EQ("asdf$:$ \"$$\\bar", result);
}

TEST(Escape, Depfile) {
  EscapeOptions opts;
  opts.mode = ESCAPE_DEPFILE;
  std::string result = EscapeString("asdf:$ \\#*[|]bar", opts, nullptr);
  EXPECT_EQ("asdf:$$\\ \\\\\\#\\*\\[\\|\\]bar", result);
}

TEST(Escape, WindowsCommand) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_COMMAND;
  opts.platform = ESCAPE_PLATFORM_WIN;

  // Regular string is passed, even if it has backslashes.
  EXPECT_EQ("foo\\bar", EscapeString("foo\\bar", opts, nullptr));

  // Spaces means the string is quoted, normal backslahes untouched.
  bool needs_quoting = false;
  EXPECT_EQ("\"foo\\$ bar\"", EscapeString("foo\\ bar", opts, &needs_quoting));
  EXPECT_TRUE(needs_quoting);

  // Inhibit quoting.
  needs_quoting = false;
  opts.inhibit_quoting = true;
  EXPECT_EQ("foo\\$ bar", EscapeString("foo\\ bar", opts, &needs_quoting));
  EXPECT_TRUE(needs_quoting);
  opts.inhibit_quoting = false;

  // Backslashes at the end of the string get escaped.
  EXPECT_EQ("\"foo$ bar\\\\\\\\\"", EscapeString("foo bar\\\\", opts, nullptr));

  // Backslashes preceding quotes are escaped, and the quote is escaped.
  EXPECT_EQ("\"foo\\\\\\\"$ bar\"", EscapeString("foo\\\" bar", opts, nullptr));
}

TEST(Escape, PosixCommand) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_COMMAND;
  opts.platform = ESCAPE_PLATFORM_POSIX;

  // : and $ ninja escaped with $. Then Shell-escape backslashes and quotes.
  EXPECT_EQ("a$:\\$ \\\"\\$$\\\\b", EscapeString("a: \"$\\b", opts, nullptr));

  // Some more generic shell chars.
  EXPECT_EQ("a_\\;\\<\\*b", EscapeString("a_;<*b", opts, nullptr));

  // Curly braces must be escaped to avoid brace expansion on systems using
  // bash as default shell..
  EXPECT_EQ("\\{a,b\\}\\{c,d\\}", EscapeString("{a,b}{c,d}", opts, nullptr));
}

TEST(Escape, NinjaPreformatted) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_PREFORMATTED_COMMAND;

  // Only $ is escaped.
  EXPECT_EQ("a: \"$$\\b<;", EscapeString("a: \"$\\b<;", opts, nullptr));
}

TEST(Escape, Space) {
  EscapeOptions opts;
  opts.mode = ESCAPE_SPACE;

  // ' ' is escaped.
  EXPECT_EQ("-VERSION=\"libsrtp2\\ 2.1.0-pre\"",
            EscapeString("-VERSION=\"libsrtp2 2.1.0-pre\"", opts, nullptr));
}

TEST(EscapeJSONString, NinjaPreformatted) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_PREFORMATTED_COMMAND;
  opts.inhibit_quoting = true;

  StringOutputBuffer buffer;
  std::ostream out(&buffer);

  EscapeJSONStringToStream(out, "foo\\\" bar", opts);
  EXPECT_EQ("foo\\\\\\\" bar", buffer.str());

  StringOutputBuffer buffer1;
  std::ostream out1(&buffer1);
  EscapeJSONStringToStream(out1, "foo bar\\\\", opts);
  EXPECT_EQ("foo bar\\\\\\\\", buffer1.str());

  StringOutputBuffer buffer2;
  std::ostream out2(&buffer2);
  EscapeJSONStringToStream(out2, "a: \"$\\b", opts);
  EXPECT_EQ("a: \\\"$$\\\\b", buffer2.str());
}

TEST(Escape, CompilationDatabase) {
  EscapeOptions opts;
  opts.mode = ESCAPE_COMPILATION_DATABASE;

  // The only special characters are '"' and '\'.
  std::string result = EscapeString("asdf:$ \\#*[|]bar", opts, nullptr);
  EXPECT_EQ("\"asdf:$ \\\\#*[|]bar\"", result);
}
