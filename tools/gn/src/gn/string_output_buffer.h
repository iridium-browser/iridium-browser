// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_STRING_OUTPUT_BUFFER_H_
#define TOOLS_GN_STRING_OUTPUT_BUFFER_H_

#include <array>
#include <memory>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

namespace base {
class FilePath;
}  // namespace base

class Err;

// An append-only very large storage area for string data. Useful for the parts
// of GN that need to generate huge output files (e.g. --ide=json will create
// a 139 MiB project.json file for the Fuchsia build).
//
// Usage is the following:
//
//   1) Create instance.
//
//   2) Use operator<<, or Append() to append data to the instance.
//
//   3) Alternatively, create an std::ostream that takes its address as
//      argument, then use the output stream as usual to append data to it.
//
//      StringOutputBuffer storage;
//      std::ostream out(&storage);
//      out << "Hello world!";
//
//   4) Use ContentsEqual() to compare the instance's content with that of a
//      given file.
//
//   5) Use WriteToFile() to write the content to a given file.
//
class StringOutputBuffer : public std::streambuf {
 public:
  StringOutputBuffer() = default;

  // Convert content to single std::string instance. Useful for unit-testing.
  std::string str() const;

  // Return the number of characters stored in this instance.
  size_t size() const { return (pages_.size() - 1u) * kPageSize + pos_; }

  // Append string to this instance.
  void Append(const char* str, size_t len);
  void Append(std::string_view str);
  void Append(char c);

  StringOutputBuffer& operator<<(std::string_view str) {
    Append(str);
    return *this;
  }

  // Compare the content of this instance with that of the file at |file_path|.
  bool ContentsEqual(const base::FilePath& file_path) const;

  // Write the contents of this instance to a file at |file_path|.
  bool WriteToFile(const base::FilePath& file_path, Err* err) const;

  // Write the contents of this instance to a file at |file_path| unless the
  // file already exists and the contents are equal.
  bool WriteToFileIfChanged(const base::FilePath& file_path, Err* err) const;

  static size_t GetPageSizeForTesting() { return kPageSize; }

 protected:
  // Called by std::ostream to write |n| chars from |s|.
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    Append(s, static_cast<size_t>(n));
    return n;
  }

  // Called by std::ostream to write a single character.
  int_type overflow(int_type ch) override {
    Append(static_cast<char>(ch));
    return 1;
  }

 private:
  // Return the number of free bytes in the current page.
  size_t page_free_size() const { return kPageSize - pos_; }

  static constexpr size_t kPageSize = 65536;
  using Page = std::array<char, kPageSize>;

  size_t pos_ = kPageSize;
  std::vector<std::unique_ptr<Page>> pages_;
};

#endif  // TOOLS_GN_STRING_OUTPUT_BUFFER_H_
