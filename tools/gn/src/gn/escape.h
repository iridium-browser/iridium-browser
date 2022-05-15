// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_ESCAPE_H_
#define TOOLS_GN_ESCAPE_H_

#include <iosfwd>
#include <string_view>

enum EscapingMode {
  // No escaping.
  ESCAPE_NONE,

  // Space only.
  ESCAPE_SPACE,

  // Ninja string escaping.
  ESCAPE_NINJA,

  // Ninja/makefile depfile string escaping.
  ESCAPE_DEPFILE,

  // For writing commands to ninja files. This assumes the output is "one
  // thing" like a filename, so will escape or quote spaces as necessary for
  // both Ninja and the shell to keep that thing together.
  ESCAPE_NINJA_COMMAND,

  // For writing preformatted shell commands to Ninja files. This assumes the
  // output already has the proper quoting and may include special shell
  // characters which we want to pass to the shell (like when writing tool
  // commands). Only Ninja "$" are escaped.
  ESCAPE_NINJA_PREFORMATTED_COMMAND,

  // Shell escaping as described by JSON Compilation Database spec:
  // Parameters use shell quoting and shell escaping of quotes, with ‘"’ and ‘\’
  // being the only special characters.
  ESCAPE_COMPILATION_DATABASE,
};

enum EscapingPlatform {
  // Do escaping for the current platform.
  ESCAPE_PLATFORM_CURRENT,

  // Force escaping for the given platform.
  ESCAPE_PLATFORM_POSIX,
  ESCAPE_PLATFORM_WIN,
};

struct EscapeOptions {
  EscapingMode mode = ESCAPE_NONE;

  // Controls how "fork" escaping is done. You will generally want to keep the
  // default "current" platform.
  EscapingPlatform platform = ESCAPE_PLATFORM_CURRENT;

  // When the escaping mode is ESCAPE_SHELL, the escaper will normally put
  // quotes around things with spaces. If this value is set to true, we'll
  // disable the quoting feature and just add the spaces.
  //
  // This mode is for when quoting is done at some higher-level. Defaults to
  // false. Note that Windows has strange behavior where the meaning of the
  // backslashes changes according to if it is followed by a quote. The
  // escaping rules assume that a double-quote will be appended to the result.
  bool inhibit_quoting = false;
};

// Escapes the given input, returnining the result.
//
// If needed_quoting is non-null, whether the string was or should have been
// (if inhibit_quoting was set) quoted will be written to it. This value should
// be initialized to false by the caller and will be written to only if it's
// true (the common use-case is for chaining calls).
std::string EscapeString(std::string_view str,
                         const EscapeOptions& options,
                         bool* needed_quoting);

// Same as EscapeString but writes the results to the given stream, saving a
// copy.
void EscapeStringToStream(std::ostream& out,
                          std::string_view str,
                          const EscapeOptions& options);

// Same as EscapeString but escape JSON string and writes the results to the
// given stream, saving a copy.
void EscapeJSONStringToStream(std::ostream& out,
                              std::string_view str,
                              const EscapeOptions& options);

#endif  // TOOLS_GN_ESCAPE_H_
