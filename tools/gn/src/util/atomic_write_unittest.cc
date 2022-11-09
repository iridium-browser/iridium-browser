// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/atomic_write.h"

#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "util/test/test.h"

class ImportantFileWriterTest : public testing::Test {
 public:
  ImportantFileWriterTest() = default;
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    file_ = temp_dir_.GetPath().AppendASCII("test-file");
  }

 protected:
  base::FilePath file_;

 private:
  base::ScopedTempDir temp_dir_;
};

// Test that WriteFileAtomically works.
TEST_F(ImportantFileWriterTest, Basic) {
  const std::string data = "Test string for writing.";
  EXPECT_FALSE(base::PathExists(file_));
  EXPECT_TRUE(util::WriteFileAtomically(file_, data.data(), data.size()));
  std::string actual;
  EXPECT_TRUE(ReadFileToString(file_, &actual));
  EXPECT_EQ(data, actual);
}
