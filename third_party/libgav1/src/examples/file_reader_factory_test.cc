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

#include "examples/file_reader_factory.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "absl/memory/memory.h"
#include "examples/file_reader_interface.h"
#include "gtest/gtest.h"

namespace libgav1 {
namespace {

class AlwaysFailFileReader : public FileReaderInterface {
 public:
  static std::unique_ptr<FileReaderInterface> Open(
      const std::string& /*file_name*/, bool /*error_tolerant*/) {
    return nullptr;
  }

  AlwaysFailFileReader() = delete;
  AlwaysFailFileReader(const AlwaysFailFileReader&) = delete;
  AlwaysFailFileReader& operator=(const AlwaysFailFileReader&) = delete;
  // Note this isn't overridden as the class can never be instantiated. This
  // avoids an unused function warning.
  // ~AlwaysFailFileReader() override = default;

  bool ReadTemporalUnit(std::vector<uint8_t>* /*data*/,
                        int64_t* /*pts*/) override {
    return false;
  }
  bool IsEndOfFile() const override { return false; }

  size_t width() const override { return 0; }
  size_t height() const override { return 0; }
  size_t frame_rate() const override { return 0; }
  size_t time_scale() const override { return 0; }

  static bool is_registered_;
};

class AlwaysOkFileReader : public FileReaderInterface {
 public:
  static std::unique_ptr<FileReaderInterface> Open(
      const std::string& /*file_name*/, bool /*error_tolerant*/) {
    auto reader = absl::WrapUnique(new (std::nothrow) AlwaysOkFileReader());

    return reader;
  }

  AlwaysOkFileReader(const AlwaysOkFileReader&) = delete;
  AlwaysOkFileReader& operator=(const AlwaysOkFileReader&) = delete;
  ~AlwaysOkFileReader() override = default;

  bool ReadTemporalUnit(std::vector<uint8_t>* /*data*/,
                        int64_t* /*pts*/) override {
    return true;
  }
  bool IsEndOfFile() const override { return true; }

  size_t width() const override { return 1; }
  size_t height() const override { return 1; }
  size_t frame_rate() const override { return 1; }
  size_t time_scale() const override { return 1; }

  static bool is_registered_;

 private:
  AlwaysOkFileReader() = default;
};

bool AlwaysFailFileReader::is_registered_ =
    FileReaderFactory::RegisterReader(AlwaysFailFileReader::Open);

bool AlwaysOkFileReader::is_registered_ =
    FileReaderFactory::RegisterReader(AlwaysOkFileReader::Open);

TEST(FileReaderFactoryTest, RegistrationFail) {
  EXPECT_FALSE(FileReaderFactory::RegisterReader(nullptr));
}

TEST(FileReaderFactoryTest, OpenReader) {
  ASSERT_TRUE(AlwaysOkFileReader::is_registered_);
  ASSERT_TRUE(AlwaysFailFileReader::is_registered_);

  auto reader = FileReaderFactory::OpenReader("fake file");
  EXPECT_NE(reader, nullptr);
  EXPECT_TRUE(reader->IsEndOfFile());
  EXPECT_TRUE(reader->ReadTemporalUnit(nullptr, nullptr));
  EXPECT_EQ(reader->width(), 1);
  EXPECT_EQ(reader->height(), 1);
  EXPECT_EQ(reader->frame_rate(), 1);
  EXPECT_EQ(reader->time_scale(), 1);
}

}  // namespace
}  // namespace libgav1
