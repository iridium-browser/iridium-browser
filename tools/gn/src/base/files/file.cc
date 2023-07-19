// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "util/build_config.h"

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
#include <errno.h>
#endif

namespace base {

File::Info::Info() = default;

File::Info::~Info() = default;

File::File() = default;

File::File(const FilePath& path, uint32_t flags)
    : error_details_(FILE_OK) {
  Initialize(path, flags);
}

File::File(ScopedPlatformFile platform_file)
    : file_(std::move(platform_file)),
      error_details_(FILE_OK) {
#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  DCHECK_GE(file_.get(), -1);
#endif
}

File::File(PlatformFile platform_file)
    : file_(platform_file),
      error_details_(FILE_OK) {
#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  DCHECK_GE(platform_file, -1);
#endif
}

File::File(Error error_details)
    : error_details_(error_details) {}

File::File(File&& other)
    : file_(other.TakePlatformFile()),
      error_details_(other.error_details()) {}

File::~File() {
  // Go through the AssertIOAllowed logic.
  Close();
}

File& File::operator=(File&& other) {
  Close();
  SetPlatformFile(other.TakePlatformFile());
  error_details_ = other.error_details();
  return *this;
}

void File::Initialize(const FilePath& path, uint32_t flags) {
  if (path.ReferencesParent()) {
#if defined(OS_WIN)
    ::SetLastError(ERROR_ACCESS_DENIED);
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
    errno = EACCES;
#else
#error Unsupported platform
#endif
    error_details_ = FILE_ERROR_ACCESS_DENIED;
    return;
  }
  DoInitialize(path, flags);
}

std::string File::ErrorToString(Error error) {
  switch (error) {
    case FILE_OK:
      return "FILE_OK";
    case FILE_ERROR_FAILED:
      return "FILE_ERROR_FAILED";
    case FILE_ERROR_IN_USE:
      return "FILE_ERROR_IN_USE";
    case FILE_ERROR_EXISTS:
      return "FILE_ERROR_EXISTS";
    case FILE_ERROR_NOT_FOUND:
      return "FILE_ERROR_NOT_FOUND";
    case FILE_ERROR_ACCESS_DENIED:
      return "FILE_ERROR_ACCESS_DENIED";
    case FILE_ERROR_TOO_MANY_OPENED:
      return "FILE_ERROR_TOO_MANY_OPENED";
    case FILE_ERROR_NO_MEMORY:
      return "FILE_ERROR_NO_MEMORY";
    case FILE_ERROR_NO_SPACE:
      return "FILE_ERROR_NO_SPACE";
    case FILE_ERROR_NOT_A_DIRECTORY:
      return "FILE_ERROR_NOT_A_DIRECTORY";
    case FILE_ERROR_INVALID_OPERATION:
      return "FILE_ERROR_INVALID_OPERATION";
    case FILE_ERROR_SECURITY:
      return "FILE_ERROR_SECURITY";
    case FILE_ERROR_ABORT:
      return "FILE_ERROR_ABORT";
    case FILE_ERROR_NOT_A_FILE:
      return "FILE_ERROR_NOT_A_FILE";
    case FILE_ERROR_NOT_EMPTY:
      return "FILE_ERROR_NOT_EMPTY";
    case FILE_ERROR_INVALID_URL:
      return "FILE_ERROR_INVALID_URL";
    case FILE_ERROR_IO:
      return "FILE_ERROR_IO";
    case FILE_ERROR_MAX:
      break;
  }

  NOTREACHED();
  return "";
}

}  // namespace base
