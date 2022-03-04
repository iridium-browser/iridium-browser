// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_TOKENIZER_H_
#define TOOLS_GN_TOKENIZER_H_

#include <stddef.h>

#include <string_view>
#include <vector>

#include "gn/err.h"
#include "gn/token.h"

class InputFile;

// Tab (0x09), vertical tab (0x0B), and formfeed (0x0C) are illegal in GN files.
// Almost always these are errors. However, in the case of running the formatter
// it's nice to convert these to spaces when encountered so that the input can
// still be parsed and rewritten correctly by the formatter.
enum class WhitespaceTransform {
  kMaintainOriginalInput,
  kInvalidToSpace,
};

class Tokenizer {
 public:
  static std::vector<Token> Tokenize(
      const InputFile* input_file,
      Err* err,
      WhitespaceTransform whitespace_transform =
          WhitespaceTransform::kMaintainOriginalInput);

  // Counts lines in the given buffer (the first line is "1") and returns
  // the byte offset of the beginning of that line, or (size_t)-1 if there
  // aren't that many lines in the file. Note that this will return the byte
  // one past the end of the input if the last character is a newline.
  //
  // This is a helper function for error output so that the tokenizer's
  // notion of lines can be used elsewhere.
  static size_t ByteOffsetOfNthLine(std::string_view buf, int n);

  // Returns true if the given offset of the string piece counts as a newline.
  // The offset must be in the buffer.
  static bool IsNewline(std::string_view buffer, size_t offset);

  static bool IsIdentifierFirstChar(char c);

  static bool IsIdentifierContinuingChar(char c);

  static Token::Type ClassifyToken(char next_char, char following_char);

 private:
  // InputFile must outlive the tokenizer and all generated tokens.
  Tokenizer(const InputFile* input_file,
            Err* err,
            WhitespaceTransform whitespace_transform);
  ~Tokenizer();

  std::vector<Token> Run();

  void AdvanceToNextToken();
  Token::Type ClassifyCurrent() const;
  void AdvanceToEndOfToken(const Location& location, Token::Type type);

  // Whether from this location back to the beginning of the line is only
  // whitespace. |location| should be the first character of the token to be
  // checked.
  bool AtStartOfLine(size_t location) const;

  bool IsCurrentWhitespace() const;
  bool IsCurrentNewline() const;
  bool IsCurrentStringTerminator(char quote_char) const;

  bool CanIncrement() const { return cur_ < input_.size() - 1; }

  // Increments the current location by one.
  void Advance();

  // Returns the current character in the file as a location.
  Location GetCurrentLocation() const;

  Err GetErrorForInvalidToken(const Location& location) const;

  bool done() const { return at_end() || has_error(); }

  bool at_end() const { return cur_ == input_.size(); }
  char cur_char() const { return input_[cur_]; }

  bool has_error() const { return err_->has_error(); }

  std::vector<Token> tokens_;

  const InputFile* input_file_;
  const std::string_view input_;
  Err* err_;
  WhitespaceTransform whitespace_transform_;
  size_t cur_ = 0;  // Byte offset into input buffer.

  int line_number_ = 1;
  int column_number_ = 1;

  Tokenizer(const Tokenizer&) = delete;
  Tokenizer& operator=(const Tokenizer&) = delete;
};

#endif  // TOOLS_GN_TOKENIZER_H_
