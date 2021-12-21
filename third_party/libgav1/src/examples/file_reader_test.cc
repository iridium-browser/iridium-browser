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

#include "examples/file_reader.h"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "examples/file_reader_interface.h"
#include "examples/file_reader_test_common.h"
#include "gtest/gtest.h"
#include "tests/utils.h"

namespace libgav1 {
namespace {

// For use with tests that expect Open() failure to distinguish failure due to
// the file contents versus failure due to a missing file.
bool FileCanBeRead(const std::string& filename) {
  FILE* const file = fopen(filename.c_str(), "r");
  if (file != nullptr) {
    fclose(file);
    return true;
  }
  return false;
}

TEST(FileReaderTest, FailOpen) {
  EXPECT_EQ(FileReader::Open(""), nullptr);
  const std::string filename =
      test_utils::GetTestInputFilePath("ivf-signature-only");
  SCOPED_TRACE("Filename: " + filename);
  EXPECT_TRUE(FileCanBeRead(filename));
  EXPECT_EQ(FileReader::Open(filename), nullptr);
}

TEST(FileReaderTest, Open) {
  const std::string filenames[] = {
      test_utils::GetTestInputFilePath("five-frames.ivf"),
      test_utils::GetTestInputFilePath("ivf-header-and-truncated-frame-header"),
      test_utils::GetTestInputFilePath("ivf-header-only"),
      test_utils::GetTestInputFilePath("one-frame-truncated.ivf"),
      test_utils::GetTestInputFilePath("one-frame.ivf"),
  };
  for (const auto& filename : filenames) {
    EXPECT_NE(FileReader::Open(filename), nullptr) << "Filename: " << filename;
  }
}

TEST_P(FileReaderFailTest, FailRead) {
  ASSERT_FALSE(reader_->ReadTemporalUnit(&tu_data_, nullptr));
}

TEST_P(FileReaderErrorTolerant, ReadThroughEndOfFile) {
  while (!reader_->IsEndOfFile()) {
    tu_data_.clear();
    ASSERT_TRUE(reader_->ReadTemporalUnit(&tu_data_, nullptr));
    ASSERT_GT(tu_data_.size(), 0);
  }
}

TEST_P(FileReaderTestNoTimeStamps, ReadThroughEndOfFile) {
  while (!reader_->IsEndOfFile()) {
    tu_data_.clear();
    ASSERT_TRUE(reader_->ReadTemporalUnit(&tu_data_, nullptr));
  }
}

TEST_P(FileReaderTestWithTimeStamps, ReadThroughEndOfFile) {
  int64_t timestamp = 0;
  while (!reader_->IsEndOfFile()) {
    tu_data_.clear();
    ASSERT_TRUE(reader_->ReadTemporalUnit(&tu_data_, &timestamp));
    if (!tu_data_.empty()) {
      last_timestamp_ = timestamp;
    }
  }
  ASSERT_TRUE(tu_data_.empty());
  ASSERT_EQ(last_timestamp_, expected_last_timestamp_);
}

INSTANTIATE_TEST_SUITE_P(
    FailRead, FileReaderFailTest,
    testing::Values(
        FileReaderTestParameters(FileReader::Open,
                                 "ivf-header-and-truncated-frame-header"),
        FileReaderTestParameters(FileReader::Open, "one-frame-truncated.ivf")));

INSTANTIATE_TEST_SUITE_P(ReadThroughEndOfFile, FileReaderErrorTolerant,
                         testing::Values(FileReaderTestParameters(
                             FileReader::Open, "one-frame-truncated.ivf")));

INSTANTIATE_TEST_SUITE_P(
    ReadThroughEndOfFile, FileReaderTestNoTimeStamps,
    testing::Values(FileReaderTestParameters(FileReader::Open, "one-frame.ivf"),
                    FileReaderTestParameters(FileReader::Open,
                                             "one-frame-large-timestamp.ivf"),
                    FileReaderTestParameters(FileReader::Open,
                                             "five-frames.ivf")));

INSTANTIATE_TEST_SUITE_P(
    ReadThroughEndOfFile, FileReaderTestWithTimeStamps,
    testing::Values(
        FileReaderTestWithTimeStampsParameters(FileReader::Open,
                                               "one-frame.ivf", 0),
        FileReaderTestWithTimeStampsParameters(FileReader::Open,
                                               "one-frame-large-timestamp.ivf",
                                               4294967296),
        FileReaderTestWithTimeStampsParameters(FileReader::Open,
                                               "five-frames.ivf", 4)));

}  // namespace
}  // namespace libgav1
