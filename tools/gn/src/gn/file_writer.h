// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_FILE_WRITER_H_
#define TOOLS_GN_FILE_WRITER_H_

#include <string_view>

#include "util/build_config.h"

#if defined(OS_WIN)
#include "base/win/scoped_handle.h"
#else
#include "base/files/scoped_file.h"
#endif

#include <string>

namespace base {
class FilePath;
}

// Convenience class to write data to a file. This is used to work around two
// limitations of base::WriteFile, i.e.:
//
//  - base::WriteFile() doesn't allow multiple writes to the target file.
//
//
//  - Windows-specific issues created by anti-virus programs required opening
//    the file differently (see http://crbug.com/468437).
//
// Usage is:
//   1) Create instance.
//   2) Call Create() to create the file.
//   3) Call Write() one or more times to write data to it.
//   4) Call Close(), or the destructor, to close the file.
//
// As soon as one method fails, all other calls will return false, this allows
// simplified error checking as in:
//
//      FileWriter writer;
//      writer.Create(<some_path>);
//      writer.Write(<some_data>);
//      writer.Write(<some_more_data>);
//      if (!writer.Close()) {
//         ... error happened in one of the above calls.
//      }
//
class FileWriter {
 public:
  FileWriter() = default;
  ~FileWriter();

  // Create output file. Return true on success, false otherwise.
  bool Create(const base::FilePath& file_path);

  // Append |data| to the output file. Return true on success, or false on
  // failure or if any previous Create() or Write() call failed.
  bool Write(std::string_view data);

  // Close the file. Return true on success, or false on failure or if
  // any previous Create() or Write() call failed.
  bool Close();

 private:
#if defined(OS_WIN)
  base::win::ScopedHandle file_;
  std::string file_path_;
#else
  base::ScopedFD fd_;
#endif
  bool valid_ = true;
};

#endif  // TOOLS_GN_FILE_WRITER_H_
