// Copyright 2021 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/gav1/version.h"

#include <regex>  // NOLINT (unapproved c++11 header)

#include "gtest/gtest.h"

namespace libgav1 {
namespace {

TEST(VersionTest, GetVersion) {
  const int library_version = GetVersion();
  EXPECT_EQ((library_version >> 24) & 0xff, 0);
  // Note if we link against a shared object there's potential for a mismatch
  // if a different library is loaded at runtime.
  EXPECT_EQ((library_version >> 16) & 0xff, LIBGAV1_MAJOR_VERSION);
  EXPECT_EQ((library_version >> 8) & 0xff, LIBGAV1_MINOR_VERSION);
  EXPECT_EQ(library_version & 0xff, LIBGAV1_PATCH_VERSION);

  const int header_version = LIBGAV1_VERSION;
  EXPECT_EQ((header_version >> 24) & 0xff, 0);
  EXPECT_EQ((header_version >> 16) & 0xff, LIBGAV1_MAJOR_VERSION);
  EXPECT_EQ((header_version >> 8) & 0xff, LIBGAV1_MINOR_VERSION);
  EXPECT_EQ(header_version & 0xff, LIBGAV1_PATCH_VERSION);
}

TEST(VersionTest, GetVersionString) {
  const char* version = GetVersionString();
  ASSERT_NE(version, nullptr);
  // https://semver.org/#is-there-a-suggested-regular-expression-regex-to-check-a-semver-string
  const std::regex semver_regex(
      R"(^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*))"
      R"((?:-((?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))"
      R"((?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?)"
      R"((?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$)");

  EXPECT_TRUE(std::regex_match(version, semver_regex)) << version;
  // Regex validation:
  // It shouldn't accept a version starting with a non-digit.
  version = "v1.2.3";
  EXPECT_FALSE(std::regex_match(version, semver_regex)) << version;
  // It shouldn't accept a version with spaces."
  version = "1.2.3 alpha";
  EXPECT_FALSE(std::regex_match(version, semver_regex)) << version;
}

TEST(VersionTest, GetBuildConfiguration) {
  const char* config = GetBuildConfiguration();
  ASSERT_NE(config, nullptr);
}

}  // namespace
}  // namespace libgav1
