// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/token.h"

#include "base/logging.h"
#include "gn/tokenizer.h"

Token::Token() : type_(INVALID), value_() {}

Token::Token(const Location& location, Type t, std::string_view v)
    : type_(t), value_(v), location_(location) {}

// static
Token Token::ClassifyAndMake(const Location& location, std::string_view v) {
  char first = v.size() > 0 ? v[0] : '\0';
  char second = v.size() > 1 ? v[1] : '\0';
  return Token(location, Tokenizer::ClassifyToken(first, second), v);
}

bool Token::IsIdentifierEqualTo(const char* v) const {
  return type_ == IDENTIFIER && value_ == v;
}

bool Token::IsStringEqualTo(const char* v) const {
  return type_ == STRING && value_ == v;
}
