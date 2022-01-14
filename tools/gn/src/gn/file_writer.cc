// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/file_writer.h"

#include "base/files/file_path.h"
#include "base/logging.h"
#include "gn/filesystem_utils.h"

#if defined(OS_WIN)
#include <windows.h>
#include "base/strings/utf_string_conversions.h"
#else
#include <fcntl.h>
#include <unistd.h>
#include "base/posix/eintr_wrapper.h"
#endif

FileWriter::~FileWriter() = default;

#if defined(OS_WIN)

bool FileWriter::Create(const base::FilePath& file_path) {
  // On Windows, provide a custom implementation of base::WriteFile. Sometimes
  // the base version fails, especially on the bots. The guess is that Windows
  // Defender or other antivirus programs still have the file open (after
  // checking for the read) when the write happens immediately after. This
  // version opens with FILE_SHARE_READ (normally not what you want when
  // replacing the entire contents of the file) which lets us continue even if
  // another program has the file open for reading. See
  // http://crbug.com/468437
  file_path_ = base::UTF16ToUTF8(file_path.value());
  file_ = base::win::ScopedHandle(::CreateFile(
      reinterpret_cast<LPCWSTR>(file_path.value().c_str()), GENERIC_WRITE,
      FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL));

  valid_ = file_.IsValid();
  if (!valid_) {
    PLOG(ERROR) << "CreateFile failed for path " << file_path_;
  }
  return valid_;
}

bool FileWriter::Write(std::string_view str) {
  if (!valid_)
    return false;

  DWORD written;
  BOOL result =
      ::WriteFile(file_.Get(), str.data(), str.size(), &written, nullptr);
  if (!result) {
    PLOG(ERROR) << "writing file " << file_path_ << " failed";
    valid_ = false;
    return false;
  }
  if (static_cast<size_t>(written) != str.size()) {
    PLOG(ERROR) << "wrote " << written << " bytes to "
                << file_path_ << " expected " << str.size();
    valid_ = false;
    return false;
  }
  return true;
}

bool FileWriter::Close() {
  // NOTE: file_.Close() is not used here because it cannot return an error.
  HANDLE handle = file_.Take();
  if (handle && !::CloseHandle(handle))
    return false;

  return valid_;
}

#else  // !OS_WIN

bool FileWriter::Create(const base::FilePath& file_path) {
  fd_.reset(HANDLE_EINTR(::creat(file_path.value().c_str(), 0666)));
  valid_ = fd_.is_valid();
  if (!valid_) {
    PLOG(ERROR) << "creat() failed for path " << file_path.value();
  }
  return valid_;
}

bool FileWriter::Write(std::string_view str) {
  if (!valid_)
    return false;

  while (!str.empty()) {
    ssize_t written = HANDLE_EINTR(::write(fd_.get(), str.data(), str.size()));
    if (written <= 0) {
      valid_ = false;
      return false;
    }
    str.remove_prefix(static_cast<size_t>(written));
  }
  return true;
}

bool FileWriter::Close() {
  // The ScopedFD reset() method will crash on EBADF and ignore other errors
  // intentionally, so no need to check anything here.
  fd_.reset();
  return valid_;
}

#endif  // !OS_WIN
