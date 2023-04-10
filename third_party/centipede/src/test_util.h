// Copyright 2022 The Centipede Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_CENTIPEDE_INTERNAL_TEST_UTIL_H_
#define THIRD_PARTY_CENTIPEDE_INTERNAL_TEST_UTIL_H_

#include <filesystem>
#include <string>

#include "./logging.h"

#define EXPECT_OK(status) EXPECT_TRUE((status).ok()) << VV(status)
#define ASSERT_OK(status) ASSERT_TRUE((status).ok()) << VV(status)

namespace centipede {

// Returns a temp dir for use inside tests. The base dir is chosen in the
// following order of precedence:
// - $TEST_TMPDIR (highest)
// - $TMPDIR
// - /tmp
//
// An optional `subdir` can be appended to the base dir chosen as above. One
// useful value always available inside a TEST macro (and its variations) is
// `test_into_->name()`, which returns the name of the test case.
//
// If the final dir doesn't exist, it gets created.
std::string GetTestTempDir(std::string_view subdir = "");

// Returns the root directory filepath for a test's "runfiles".
std::filesystem::path GetTestRunfilesDir();

// Returns the filepath of a test's data dependency file.
std::filesystem::path GetDataDependencyFilepath(std::string_view rel_path);

// Resets the PATH envvar to "`dir`:$PATH".
void PrependDirToPathEnvvar(std::string_view dir);

}  // namespace centipede

#endif  // THIRD_PARTY_CENTIPEDE_INTERNAL_TEST_UTIL_H_
