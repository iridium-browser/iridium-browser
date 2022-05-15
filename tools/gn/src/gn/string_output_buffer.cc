// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/string_output_buffer.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "gn/err.h"
#include "gn/file_writer.h"
#include "gn/filesystem_utils.h"

#include <fstream>

std::string StringOutputBuffer::str() const {
  std::string result;
  size_t data_size = size();
  result.reserve(data_size);
  for (size_t nn = 0; nn < pages_.size(); ++nn) {
    size_t wanted_size = std::min(kPageSize, data_size - nn * kPageSize);
    result.append(pages_[nn]->data(), wanted_size);
  }
  return result;
}

void StringOutputBuffer::Append(const char* str, size_t len) {
  Append(std::string_view(str, len));
}

void StringOutputBuffer::Append(std::string_view str) {
  while (str.size() > 0) {
    if (page_free_size() == 0) {
      // Allocate a new page.
      pages_.push_back(std::make_unique<Page>());
      pos_ = 0;
    }
    size_t size = std::min(page_free_size(), str.size());
    memcpy(pages_.back()->data() + pos_, str.data(), size);
    pos_ += size;
    str.remove_prefix(size);
  }
}

void StringOutputBuffer::Append(char c) {
  if (page_free_size() == 0) {
    // Allocate a new page.
    pages_.push_back(std::make_unique<Page>());
    pos_ = 0;
  }
  pages_.back()->data()[pos_] = c;
  pos_ += 1;
}

bool StringOutputBuffer::ContentsEqual(const base::FilePath& file_path) const {
  // Compare file and stream sizes first. Quick and will save us some time if
  // they are different sizes.
  size_t data_size = size();
  int64_t file_size;
  if (!base::GetFileSize(file_path, &file_size) ||
      static_cast<size_t>(file_size) != data_size) {
    return false;
  }

  // Open the file in binary mode.
  std::ifstream file(file_path.As8Bit().c_str(), std::ios::binary);
  if (!file.is_open())
    return false;

  size_t page_count = pages_.size();
  Page file_page;
  for (size_t nn = 0; nn < page_count; ++nn) {
    size_t wanted_size = std::min(data_size - nn * kPageSize, kPageSize);
    file.read(file_page.data(), wanted_size);
    if (!file.good())
      return false;

    if (memcmp(file_page.data(), pages_[nn]->data(), wanted_size) != 0)
      return false;
  }
  return true;
}

// Write the contents of this instance to a file at |file_path|.
bool StringOutputBuffer::WriteToFile(const base::FilePath& file_path,
                                     Err* err) const {
  // Create the directory if necessary.
  if (!base::CreateDirectory(file_path.DirName())) {
    if (err) {
      *err =
          Err(Location(), "Unable to create directory.",
              "I was using \"" + FilePathToUTF8(file_path.DirName()) + "\".");
    }
    return false;
  }

  size_t data_size = size();
  size_t page_count = pages_.size();

  FileWriter writer;
  bool success = writer.Create(file_path);
  if (success) {
    for (size_t nn = 0; nn < page_count; ++nn) {
      size_t wanted_size = std::min(data_size - nn * kPageSize, kPageSize);
      success = writer.Write(std::string_view(pages_[nn]->data(), wanted_size));
      if (!success)
        break;
    }
  }
  if (!writer.Close())
    success = false;

  if (!success && err) {
    *err = Err(Location(), "Unable to write file.",
               "I was writing \"" + FilePathToUTF8(file_path) + "\".");
  }
  return success;
}

bool StringOutputBuffer::WriteToFileIfChanged(const base::FilePath& file_path,
                                              Err* err) const {
  if (ContentsEqual(file_path))
    return true;

  return WriteToFile(file_path, err);
}
