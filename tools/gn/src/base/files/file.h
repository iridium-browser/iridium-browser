// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_FILES_FILE_H_
#define BASE_FILES_FILE_H_

#include <stdint.h>

#include <string>

#include "base/files/file_path.h"
#include "base/files/platform_file.h"
#include "base/files/scoped_file.h"
#include "util/build_config.h"
#include "util/ticks.h"

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
#include <sys/stat.h>
#endif

namespace base {

#if defined(OS_BSD) || defined(OS_MACOSX) || defined(OS_NACL) || \
    defined(OS_HAIKU) || defined(OS_MSYS) || defined(OS_ZOS) ||  \
    defined(OS_ANDROID) && __ANDROID_API__ < 21
typedef struct stat stat_wrapper_t;
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
typedef struct stat64 stat_wrapper_t;
#endif

// Thin wrapper around an OS-level file.
// Note that this class does not provide any support for asynchronous IO.
//
// Note about const: this class does not attempt to determine if the underlying
// file system object is affected by a particular method in order to consider
// that method const or not. Only methods that deal with member variables in an
// obvious non-modifying way are marked as const. Any method that forward calls
// to the OS is not considered const, even if there is no apparent change to
// member variables.
class File {
 public:
  // FLAG_(OPEN|CREATE).* are mutually exclusive. You should specify exactly one
  // of the three (possibly combining with other flags) when opening or creating
  // a file.
  enum Flags {
    FLAG_OPEN = 1 << 0,           // Opens a file, only if it exists.
    FLAG_CREATE = 1 << 1,         // Creates a new file, only if it does not
                                  // already exist.
    FLAG_CREATE_ALWAYS = 1 << 3,  // May overwrite an old file.
    FLAG_READ = 1 << 4,
    FLAG_WRITE = 1 << 5,
  };

  // This enum has been recorded in multiple histograms using PlatformFileError
  // enum. If the order of the fields needs to change, please ensure that those
  // histograms are obsolete or have been moved to a different enum.
  //
  // FILE_ERROR_ACCESS_DENIED is returned when a call fails because of a
  // filesystem restriction. FILE_ERROR_SECURITY is returned when a browser
  // policy doesn't allow the operation to be executed.
  enum Error {
    FILE_OK = 0,
    FILE_ERROR_FAILED = -1,
    FILE_ERROR_IN_USE = -2,
    FILE_ERROR_EXISTS = -3,
    FILE_ERROR_NOT_FOUND = -4,
    FILE_ERROR_ACCESS_DENIED = -5,
    FILE_ERROR_TOO_MANY_OPENED = -6,
    FILE_ERROR_NO_MEMORY = -7,
    FILE_ERROR_NO_SPACE = -8,
    FILE_ERROR_NOT_A_DIRECTORY = -9,
    FILE_ERROR_INVALID_OPERATION = -10,
    FILE_ERROR_SECURITY = -11,
    FILE_ERROR_ABORT = -12,
    FILE_ERROR_NOT_A_FILE = -13,
    FILE_ERROR_NOT_EMPTY = -14,
    FILE_ERROR_INVALID_URL = -15,
    FILE_ERROR_IO = -16,
    // Put new entries here and increment FILE_ERROR_MAX.
    FILE_ERROR_MAX = -17
  };

  // This explicit mapping matches both FILE_ on Windows and SEEK_ on Linux.
  enum Whence { FROM_BEGIN = 0, FROM_CURRENT = 1, FROM_END = 2 };

  // Used to hold information about a given file.
  // If you add more fields to this structure (platform-specific fields are OK),
  // make sure to update all functions that use it in file_util_{win|posix}.cc,
  // too, and the ParamTraits<base::File::Info> implementation in
  // ipc/ipc_message_utils.cc.
  struct Info {
    Info();
    ~Info();
#if defined(OS_POSIX) || defined(OS_FUCHSIA)
    // Fills this struct with values from |stat_info|.
    void FromStat(const stat_wrapper_t& stat_info);
#endif

    // The size of the file in bytes.  Undefined when is_directory is true.
    int64_t size = 0;

    // True if the file corresponds to a directory.
    bool is_directory = false;

    // True if the file corresponds to a symbolic link.  For Windows currently
    // not supported and thus always false.
    bool is_symbolic_link = false;

    // The last modified time of a file.
    Ticks last_modified;

    // The last accessed time of a file.
    Ticks last_accessed;

    // The creation time of a file.
    Ticks creation_time;
  };

  File();

  // Creates or opens the given file. This will fail with 'access denied' if the
  // |path| contains path traversal ('..') components.
  File(const FilePath& path, uint32_t flags);

  // Takes ownership of |platform_file|.
  explicit File(ScopedPlatformFile platform_file);
  explicit File(PlatformFile platform_file);

  // Creates an object with a specific error_details code.
  explicit File(Error error_details);

  File(File&& other);

  ~File();

  File& operator=(File&& other);

  // Creates or opens the given file.
  void Initialize(const FilePath& path, uint32_t flags);

  // Returns |true| if the handle / fd wrapped by this object is valid.  This
  // method doesn't interact with the file system (and is safe to be called from
  // ThreadRestrictions::SetIOAllowed(false) threads).
  bool IsValid() const;

  // Returns the OS result of opening this file. Note that the way to verify
  // the success of the operation is to use IsValid(), not this method:
  //   File file(path, flags);
  //   if (!file.IsValid())
  //     return;
  Error error_details() const { return error_details_; }

  PlatformFile GetPlatformFile() const;
  PlatformFile TakePlatformFile();

  // Destroying this object closes the file automatically.
  void Close();

  // Changes current position in the file to an |offset| relative to an origin
  // defined by |whence|. Returns the resultant current position in the file
  // (relative to the start) or -1 in case of error.
  int64_t Seek(Whence whence, int64_t offset);

  // Reads the given number of bytes (or until EOF is reached) starting with the
  // given offset. Returns the number of bytes read, or -1 on error. Note that
  // this function makes a best effort to read all data on all platforms, so it
  // is not intended for stream oriented files but instead for cases when the
  // normal expectation is that actually |size| bytes are read unless there is
  // an error.
  int Read(int64_t offset, char* data, int size);

  // Same as above but without seek.
  int ReadAtCurrentPos(char* data, int size);

  // Reads the given number of bytes (or until EOF is reached) starting with the
  // given offset, but does not make any effort to read all data on all
  // platforms. Returns the number of bytes read, or -1 on error.
  int ReadNoBestEffort(int64_t offset, char* data, int size);

  // Same as above but without seek.
  int ReadAtCurrentPosNoBestEffort(char* data, int size);

  // Writes the given buffer into the file at the given offset, overwriting any
  // data that was previously there. Returns the number of bytes written, or -1
  // on error. Note that this function makes a best effort to write all data on
  // all platforms. |data| can be nullptr when |size| is 0.
  int Write(int64_t offset, const char* data, int size);

  // Save as above but without seek.
  int WriteAtCurrentPos(const char* data, int size);

  // Save as above but does not make any effort to write all data on all
  // platforms. Returns the number of bytes written, or -1 on error.
  int WriteAtCurrentPosNoBestEffort(const char* data, int size);

  // Returns the current size of this file, or a negative number on failure.
  int64_t GetLength();

  // Truncates the file to the given length. If |length| is greater than the
  // current size of the file, the file is extended with zeros. If the file
  // doesn't exist, |false| is returned.
  bool SetLength(int64_t length);

  // Instructs the filesystem to flush the file to disk. (POSIX: fsync, Windows:
  // FlushFileBuffers).
  // Calling Flush() does not guarantee file integrity and thus is not a valid
  // substitute for file integrity checks and recovery codepaths for malformed
  // files. It can also be *really* slow, so avoid blocking on Flush(),
  // especially please don't block shutdown on Flush().
  // Latency percentiles of Flush() across all platforms as of July 2016:
  // 50 %     > 5 ms
  // 10 %     > 58 ms
  //  1 %     > 357 ms
  //  0.1 %   > 1.8 seconds
  //  0.01 %  > 7.6 seconds
  bool Flush();

  // Returns some basic information for the given file.
  bool GetInfo(Info* info);

#if !defined(OS_FUCHSIA)  // Fuchsia's POSIX API does not support file locking.

  // Attempts to take an exclusive write lock on the file. Returns immediately
  // (i.e. does not wait for another process to unlock the file). If the lock
  // was obtained, the result will be FILE_OK. A lock only guarantees
  // that other processes may not also take a lock on the same file with the
  // same API - it may still be opened, renamed, unlinked, etc.
  //
  // Common semantics:
  //  * Locks are held by processes, but not inherited by child processes.
  //  * Locks are released by the OS on file close or process termination.
  //  * Locks are reliable only on local filesystems.
  //  * Duplicated file handles may also write to locked files.
  // Windows-specific semantics:
  //  * Locks are mandatory for read/write APIs, advisory for mapping APIs.
  //  * Within a process, locking the same file (by the same or new handle)
  //    will fail.
  // POSIX-specific semantics:
  //  * Locks are advisory only.
  //  * Within a process, locking the same file (by the same or new handle)
  //    will succeed.
  //  * Closing any descriptor on a given file releases the lock.
  Error Lock();

  // Unlock a file previously locked.
  Error Unlock();

#endif  // !defined(OS_FUCHSIA)

  // Returns a new object referencing this file for use within the current
  // process.
  File Duplicate() const;

#if defined(OS_WIN)
  static Error OSErrorToFileError(DWORD last_error);
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
  static Error OSErrorToFileError(int saved_errno);
#endif

  // Gets the last global error (errno or GetLastError()) and converts it to the
  // closest base::File::Error equivalent via OSErrorToFileError(). The returned
  // value is only trustworthy immediately after another base::File method
  // fails. base::File never resets the global error to zero.
  static Error GetLastFileError();

  // Converts an error value to a human-readable form. Used for logging.
  static std::string ErrorToString(Error error);

 private:
  // Creates or opens the given file. Only called if |path| has no
  // traversal ('..') components.
  void DoInitialize(const FilePath& path, uint32_t flags);

  void SetPlatformFile(PlatformFile file);

  ScopedPlatformFile file_;

  Error error_details_ = FILE_ERROR_FAILED;

  File(const File&) = delete;
  File& operator=(const File&) = delete;
};

}  // namespace base

#endif  // BASE_FILES_FILE_H_
