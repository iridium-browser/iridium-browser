// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/source_file.h"

#include "util/test/test.h"

// The SourceFile object should normalize the input passed to the constructor.
// The normalizer unit test checks for all the weird edge cases for normalizing
// so here just check that it gets called.
TEST(SourceFile, Normalize) {
  SourceFile a("//foo/../bar.cc");
  EXPECT_EQ("//bar.cc", a.value());

  std::string b_str("//foo/././../bar.cc");
  SourceFile b(std::move(b_str));
  EXPECT_TRUE(b_str.empty());  // Should have been swapped in.
  EXPECT_EQ("//bar.cc", b.value());
}

TEST(SourceFile, GetType) {
  static const struct {
    std::string_view path;
    SourceFile::Type type;
  } kData[] = {
      {"", SourceFile::SOURCE_UNKNOWN},
      {"a.c", SourceFile::SOURCE_C},
      {"a.cc", SourceFile::SOURCE_CPP},
      {"a.cpp", SourceFile::SOURCE_CPP},
      {"a.cxx", SourceFile::SOURCE_CPP},
      {"a.c++", SourceFile::SOURCE_CPP},
      {"foo.h", SourceFile::SOURCE_H},
      {"foo.hh", SourceFile::SOURCE_H},
      {"foo.hpp", SourceFile::SOURCE_H},
      {"foo.inc", SourceFile::SOURCE_H},
      {"foo.inl", SourceFile::SOURCE_H},
      {"foo.ipp", SourceFile::SOURCE_H},
      {"foo.m", SourceFile::SOURCE_M},
      {"foo.mm", SourceFile::SOURCE_MM},
      {"foo.o", SourceFile::SOURCE_O},
      {"foo.obj", SourceFile::SOURCE_O},
      {"foo.S", SourceFile::SOURCE_S},
      {"foo.s", SourceFile::SOURCE_S},
      {"foo.asm", SourceFile::SOURCE_S},
      {"foo.go", SourceFile::SOURCE_GO},
      {"foo.rc", SourceFile::SOURCE_RC},
      {"foo.rs", SourceFile::SOURCE_RS},
      {"foo.def", SourceFile::SOURCE_DEF},
      {"foo.swift", SourceFile::SOURCE_SWIFT},
      {"foo.swiftmodule", SourceFile::SOURCE_SWIFTMODULE},
      {"foo.modulemap", SourceFile::SOURCE_MODULEMAP},

      // A few degenerate cases
      {"foo.obj/a", SourceFile::SOURCE_UNKNOWN},
      {"foo.cppp", SourceFile::SOURCE_UNKNOWN},
      {"cpp", SourceFile::SOURCE_UNKNOWN},
  };
  for (const auto& data : kData) {
    EXPECT_EQ(data.type, SourceFile(data.path).GetType());
  }
}
