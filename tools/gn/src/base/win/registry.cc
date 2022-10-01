// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/registry.h"

#include <shlwapi.h>
#include <stddef.h>

#include <algorithm>
#include <iterator>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/win/win_util.h"

namespace base {
namespace win {

namespace {

// RegEnumValue() reports the number of characters from the name that were
// written to the buffer, not how many there are. This constant is the maximum
// name size, such that a buffer with this size should read any name.
const DWORD MAX_REGISTRY_NAME_SIZE = 16384;

// Registry values are read as BYTE* but can have char16_t* data whose last
// char16_t is truncated. This function converts the reported |byte_size| to
// a size in char16_t that can store a truncated char16_t if necessary.
inline DWORD to_wchar_size(DWORD byte_size) {
  return (byte_size + sizeof(char16_t) - 1) / sizeof(char16_t);
}

// Mask to pull WOW64 access flags out of REGSAM access.
const REGSAM kWow64AccessMask = KEY_WOW64_32KEY | KEY_WOW64_64KEY;

}  // namespace

// RegKey ----------------------------------------------------------------------

RegKey::RegKey() : key_(NULL), wow64access_(0) {}

RegKey::RegKey(HKEY key) : key_(key), wow64access_(0) {}

RegKey::RegKey(HKEY rootkey, const char16_t* subkey, REGSAM access)
    : key_(NULL), wow64access_(0) {
  if (rootkey) {
    if (access & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK))
      Create(rootkey, subkey, access);
    else
      Open(rootkey, subkey, access);
  } else {
    DCHECK(!subkey);
    wow64access_ = access & kWow64AccessMask;
  }
}

RegKey::~RegKey() {
  Close();
}

LONG RegKey::Create(HKEY rootkey, const char16_t* subkey, REGSAM access) {
  DWORD disposition_value;
  return CreateWithDisposition(rootkey, subkey, &disposition_value, access);
}

LONG RegKey::CreateWithDisposition(HKEY rootkey,
                                   const char16_t* subkey,
                                   DWORD* disposition,
                                   REGSAM access) {
  DCHECK(rootkey && subkey && access && disposition);
  HKEY subhkey = NULL;
  LONG result = RegCreateKeyEx(rootkey, ToWCharT(subkey), 0, NULL,
                               REG_OPTION_NON_VOLATILE, access, NULL, &subhkey,
                               disposition);
  if (result == ERROR_SUCCESS) {
    Close();
    key_ = subhkey;
    wow64access_ = access & kWow64AccessMask;
  }

  return result;
}

LONG RegKey::CreateKey(const char16_t* name, REGSAM access) {
  DCHECK(name && access);
  // After the application has accessed an alternate registry view using one of
  // the [KEY_WOW64_32KEY / KEY_WOW64_64KEY] flags, all subsequent operations
  // (create, delete, or open) on child registry keys must explicitly use the
  // same flag. Otherwise, there can be unexpected behavior.
  // http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
  if ((access & kWow64AccessMask) != wow64access_) {
    NOTREACHED();
    return ERROR_INVALID_PARAMETER;
  }
  HKEY subkey = NULL;
  LONG result =
      RegCreateKeyEx(key_, ToWCharT(name), 0, NULL, REG_OPTION_NON_VOLATILE,
                     access, NULL, &subkey, NULL);
  if (result == ERROR_SUCCESS) {
    Close();
    key_ = subkey;
    wow64access_ = access & kWow64AccessMask;
  }

  return result;
}

LONG RegKey::Open(HKEY rootkey, const char16_t* subkey, REGSAM access) {
  DCHECK(rootkey && subkey && access);
  HKEY subhkey = NULL;

  LONG result = RegOpenKeyEx(rootkey, ToWCharT(subkey), 0, access, &subhkey);
  if (result == ERROR_SUCCESS) {
    Close();
    key_ = subhkey;
    wow64access_ = access & kWow64AccessMask;
  }

  return result;
}

LONG RegKey::OpenKey(const char16_t* relative_key_name, REGSAM access) {
  DCHECK(relative_key_name && access);
  // After the application has accessed an alternate registry view using one of
  // the [KEY_WOW64_32KEY / KEY_WOW64_64KEY] flags, all subsequent operations
  // (create, delete, or open) on child registry keys must explicitly use the
  // same flag. Otherwise, there can be unexpected behavior.
  // http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
  if ((access & kWow64AccessMask) != wow64access_) {
    NOTREACHED();
    return ERROR_INVALID_PARAMETER;
  }
  HKEY subkey = NULL;
  LONG result =
      RegOpenKeyEx(key_, ToWCharT(relative_key_name), 0, access, &subkey);

  // We have to close the current opened key before replacing it with the new
  // one.
  if (result == ERROR_SUCCESS) {
    Close();
    key_ = subkey;
    wow64access_ = access & kWow64AccessMask;
  }
  return result;
}

void RegKey::Close() {
  if (key_) {
    ::RegCloseKey(key_);
    key_ = NULL;
    wow64access_ = 0;
  }
}

// TODO(wfh): Remove this and other unsafe methods. See http://crbug.com/375400
void RegKey::Set(HKEY key) {
  if (key_ != key) {
    Close();
    key_ = key;
  }
}

HKEY RegKey::Take() {
  DCHECK_EQ(wow64access_, 0u);
  HKEY key = key_;
  key_ = NULL;
  return key;
}

bool RegKey::HasValue(const char16_t* name) const {
  return RegQueryValueEx(key_, ToWCharT(name), 0, NULL, NULL, NULL) ==
         ERROR_SUCCESS;
}

DWORD RegKey::GetValueCount() const {
  DWORD count = 0;
  LONG result = RegQueryInfoKey(key_, NULL, 0, NULL, NULL, NULL, NULL, &count,
                                NULL, NULL, NULL, NULL);
  return (result == ERROR_SUCCESS) ? count : 0;
}

LONG RegKey::GetValueNameAt(int index, std::u16string* name) const {
  char16_t buf[256];
  DWORD bufsize = std::size(buf);
  LONG r = ::RegEnumValue(key_, index, ToWCharT(buf), &bufsize, NULL, NULL,
                          NULL, NULL);
  if (r == ERROR_SUCCESS)
    *name = buf;

  return r;
}

LONG RegKey::DeleteKey(const char16_t* name) {
  DCHECK(key_);
  DCHECK(name);
  HKEY subkey = NULL;

  // Verify the key exists before attempting delete to replicate previous
  // behavior.
  LONG result = RegOpenKeyEx(key_, ToWCharT(name), 0,
                             READ_CONTROL | wow64access_, &subkey);
  if (result != ERROR_SUCCESS)
    return result;
  RegCloseKey(subkey);

  return RegDelRecurse(key_, std::u16string(name), wow64access_);
}

LONG RegKey::DeleteEmptyKey(const char16_t* name) {
  DCHECK(key_);
  DCHECK(name);

  HKEY target_key = NULL;
  LONG result = RegOpenKeyEx(key_, ToWCharT(name), 0, KEY_READ | wow64access_,
                             &target_key);

  if (result != ERROR_SUCCESS)
    return result;

  DWORD count = 0;
  result = RegQueryInfoKey(target_key, NULL, 0, NULL, NULL, NULL, NULL, &count,
                           NULL, NULL, NULL, NULL);

  RegCloseKey(target_key);

  if (result != ERROR_SUCCESS)
    return result;

  if (count == 0)
    return RegDeleteKeyExWrapper(key_, name, wow64access_, 0);

  return ERROR_DIR_NOT_EMPTY;
}

LONG RegKey::DeleteValue(const char16_t* value_name) {
  DCHECK(key_);
  LONG result = RegDeleteValue(key_, ToWCharT(value_name));
  return result;
}

LONG RegKey::ReadValueDW(const char16_t* name, DWORD* out_value) const {
  DCHECK(out_value);
  DWORD type = REG_DWORD;
  DWORD size = sizeof(DWORD);
  DWORD local_value = 0;
  LONG result = ReadValue(name, &local_value, &size, &type);
  if (result == ERROR_SUCCESS) {
    if ((type == REG_DWORD || type == REG_BINARY) && size == sizeof(DWORD))
      *out_value = local_value;
    else
      result = ERROR_CANTREAD;
  }

  return result;
}

LONG RegKey::ReadInt64(const char16_t* name, int64_t* out_value) const {
  DCHECK(out_value);
  DWORD type = REG_QWORD;
  int64_t local_value = 0;
  DWORD size = sizeof(local_value);
  LONG result = ReadValue(name, &local_value, &size, &type);
  if (result == ERROR_SUCCESS) {
    if ((type == REG_QWORD || type == REG_BINARY) &&
        size == sizeof(local_value))
      *out_value = local_value;
    else
      result = ERROR_CANTREAD;
  }

  return result;
}

LONG RegKey::ReadValue(const char16_t* name, std::u16string* out_value) const {
  DCHECK(out_value);
  const size_t kMaxStringLength = 1024;  // This is after expansion.
  // Use the one of the other forms of ReadValue if 1024 is too small for you.
  char16_t raw_value[kMaxStringLength];
  DWORD type = REG_SZ, size = sizeof(raw_value);
  LONG result = ReadValue(name, raw_value, &size, &type);
  if (result == ERROR_SUCCESS) {
    if (type == REG_SZ) {
      *out_value = raw_value;
    } else if (type == REG_EXPAND_SZ) {
      char16_t expanded[kMaxStringLength];
      size = ExpandEnvironmentStrings(ToWCharT(raw_value), ToWCharT(expanded),
                                      kMaxStringLength);
      // Success: returns the number of char16_t's copied
      // Fail: buffer too small, returns the size required
      // Fail: other, returns 0
      if (size == 0 || size > kMaxStringLength) {
        result = ERROR_MORE_DATA;
      } else {
        *out_value = expanded;
      }
    } else {
      // Not a string. Oops.
      result = ERROR_CANTREAD;
    }
  }

  return result;
}

LONG RegKey::ReadValue(const char16_t* name,
                       void* data,
                       DWORD* dsize,
                       DWORD* dtype) const {
  LONG result = RegQueryValueEx(key_, ToWCharT(name), 0, dtype,
                                reinterpret_cast<LPBYTE>(data), dsize);
  return result;
}

LONG RegKey::ReadValues(const char16_t* name,
                        std::vector<std::u16string>* values) {
  values->clear();

  DWORD type = REG_MULTI_SZ;
  DWORD size = 0;
  LONG result = ReadValue(name, NULL, &size, &type);
  if (result != ERROR_SUCCESS || size == 0)
    return result;

  if (type != REG_MULTI_SZ)
    return ERROR_CANTREAD;

  std::vector<char16_t> buffer(size / sizeof(char16_t));
  result = ReadValue(name, &buffer[0], &size, NULL);
  if (result != ERROR_SUCCESS || size == 0)
    return result;

  // Parse the double-null-terminated list of strings.
  // Note: This code is paranoid to not read outside of |buf|, in the case where
  // it may not be properly terminated.
  const char16_t* entry = &buffer[0];
  const char16_t* buffer_end = entry + (size / sizeof(char16_t));
  while (entry < buffer_end && entry[0] != '\0') {
    const char16_t* entry_end = std::find(entry, buffer_end, L'\0');
    values->push_back(std::u16string(entry, entry_end));
    entry = entry_end + 1;
  }
  return 0;
}

LONG RegKey::WriteValue(const char16_t* name, DWORD in_value) {
  return WriteValue(name, &in_value, static_cast<DWORD>(sizeof(in_value)),
                    REG_DWORD);
}

LONG RegKey::WriteValue(const char16_t* name, const char16_t* in_value) {
  return WriteValue(
      name, in_value,
      static_cast<DWORD>(sizeof(*in_value) * (wcslen(ToWCharT(in_value)) + 1)),
      REG_SZ);
}

LONG RegKey::WriteValue(const char16_t* name,
                        const void* data,
                        DWORD dsize,
                        DWORD dtype) {
  DCHECK(data || !dsize);

  LONG result =
      RegSetValueEx(key_, ToWCharT(name), 0, dtype,
                    reinterpret_cast<LPBYTE>(const_cast<void*>(data)), dsize);
  return result;
}

// static
LONG RegKey::RegDeleteKeyExWrapper(HKEY hKey,
                                   const char16_t* lpSubKey,
                                   REGSAM samDesired,
                                   DWORD Reserved) {
  typedef LSTATUS(WINAPI * RegDeleteKeyExPtr)(HKEY, LPCWSTR, REGSAM, DWORD);

  RegDeleteKeyExPtr reg_delete_key_ex_func =
      reinterpret_cast<RegDeleteKeyExPtr>(
          GetProcAddress(GetModuleHandleA("advapi32.dll"), "RegDeleteKeyExW"));

  if (reg_delete_key_ex_func)
    return reg_delete_key_ex_func(hKey, ToWCharT(lpSubKey), samDesired,
                                  Reserved);

  // Windows XP does not support RegDeleteKeyEx, so fallback to RegDeleteKey.
  return RegDeleteKey(hKey, ToWCharT(lpSubKey));
}

// static
LONG RegKey::RegDelRecurse(HKEY root_key,
                           const std::u16string& name,
                           REGSAM access) {
  // First, see if the key can be deleted without having to recurse.
  LONG result = RegDeleteKeyExWrapper(root_key, name.c_str(), access, 0);
  if (result == ERROR_SUCCESS)
    return result;

  HKEY target_key = NULL;
  result = RegOpenKeyEx(root_key, ToWCharT(&name), 0,
                        KEY_ENUMERATE_SUB_KEYS | access, &target_key);

  if (result == ERROR_FILE_NOT_FOUND)
    return ERROR_SUCCESS;
  if (result != ERROR_SUCCESS)
    return result;

  std::u16string subkey_name(name);

  // Check for an ending slash and add one if it is missing.
  if (!name.empty() && subkey_name[name.length() - 1] != '\\')
    subkey_name += u"\\";

  // Enumerate the keys
  result = ERROR_SUCCESS;
  const DWORD kMaxKeyNameLength = MAX_PATH;
  const size_t base_key_length = subkey_name.length();
  std::u16string key_name;
  while (result == ERROR_SUCCESS) {
    DWORD key_size = kMaxKeyNameLength;
    result = RegEnumKeyEx(target_key, 0,
                          ToWCharT(WriteInto(&key_name, kMaxKeyNameLength)),
                          &key_size, NULL, NULL, NULL, NULL);

    if (result != ERROR_SUCCESS)
      break;

    key_name.resize(key_size);
    subkey_name.resize(base_key_length);
    subkey_name += key_name;

    if (RegDelRecurse(root_key, subkey_name, access) != ERROR_SUCCESS)
      break;
  }

  RegCloseKey(target_key);

  // Try again to delete the key.
  result = RegDeleteKeyExWrapper(root_key, name.c_str(), access, 0);

  return result;
}

// RegistryValueIterator ------------------------------------------------------

RegistryValueIterator::RegistryValueIterator(HKEY root_key,
                                             const char16_t* folder_key,
                                             REGSAM wow64access)
    : name_(MAX_PATH, L'\0'), value_(MAX_PATH, L'\0') {
  Initialize(root_key, folder_key, wow64access);
}

RegistryValueIterator::RegistryValueIterator(HKEY root_key,
                                             const char16_t* folder_key)
    : name_(MAX_PATH, L'\0'), value_(MAX_PATH, L'\0') {
  Initialize(root_key, folder_key, 0);
}

void RegistryValueIterator::Initialize(HKEY root_key,
                                       const char16_t* folder_key,
                                       REGSAM wow64access) {
  DCHECK_EQ(wow64access & ~kWow64AccessMask, static_cast<REGSAM>(0));
  LONG result = RegOpenKeyEx(root_key, ToWCharT(folder_key), 0,
                             KEY_READ | wow64access, &key_);
  if (result != ERROR_SUCCESS) {
    key_ = NULL;
  } else {
    DWORD count = 0;
    result = ::RegQueryInfoKey(key_, NULL, 0, NULL, NULL, NULL, NULL, &count,
                               NULL, NULL, NULL, NULL);

    if (result != ERROR_SUCCESS) {
      ::RegCloseKey(key_);
      key_ = NULL;
    } else {
      index_ = count - 1;
    }
  }

  Read();
}

RegistryValueIterator::~RegistryValueIterator() {
  if (key_)
    ::RegCloseKey(key_);
}

DWORD RegistryValueIterator::ValueCount() const {
  DWORD count = 0;
  LONG result = ::RegQueryInfoKey(key_, NULL, 0, NULL, NULL, NULL, NULL, &count,
                                  NULL, NULL, NULL, NULL);
  if (result != ERROR_SUCCESS)
    return 0;

  return count;
}

bool RegistryValueIterator::Valid() const {
  return key_ != NULL && index_ >= 0;
}

void RegistryValueIterator::operator++() {
  --index_;
  Read();
}

bool RegistryValueIterator::Read() {
  if (Valid()) {
    DWORD capacity = static_cast<DWORD>(name_.capacity());
    DWORD name_size = capacity;
    // |value_size_| is in bytes. Reserve the last character for a NUL.
    value_size_ = static_cast<DWORD>((value_.size() - 1) * sizeof(char16_t));
    LONG result = ::RegEnumValue(
        key_, index_, ToWCharT(WriteInto(&name_, name_size)), &name_size, NULL,
        &type_, reinterpret_cast<BYTE*>(value_.data()), &value_size_);

    if (result == ERROR_MORE_DATA) {
      // Registry key names are limited to 255 characters and fit within
      // MAX_PATH (which is 260) but registry value names can use up to 16,383
      // characters and the value itself is not limited
      // (from http://msdn.microsoft.com/en-us/library/windows/desktop/
      // ms724872(v=vs.85).aspx).
      // Resize the buffers and retry if their size caused the failure.
      DWORD value_size_in_wchars = to_wchar_size(value_size_);
      if (value_size_in_wchars + 1 > value_.size())
        value_.resize(value_size_in_wchars + 1, L'\0');
      value_size_ = static_cast<DWORD>((value_.size() - 1) * sizeof(char16_t));
      name_size = name_size == capacity ? MAX_REGISTRY_NAME_SIZE : capacity;
      result = ::RegEnumValue(
          key_, index_, ToWCharT(WriteInto(&name_, name_size)), &name_size,
          NULL, &type_, reinterpret_cast<BYTE*>(value_.data()), &value_size_);
    }

    if (result == ERROR_SUCCESS) {
      DCHECK_LT(to_wchar_size(value_size_), value_.size());
      value_[to_wchar_size(value_size_)] = L'\0';
      return true;
    }
  }

  name_[0] = L'\0';
  value_[0] = L'\0';
  value_size_ = 0;
  return false;
}

// RegistryKeyIterator --------------------------------------------------------

RegistryKeyIterator::RegistryKeyIterator(HKEY root_key,
                                         const char16_t* folder_key) {
  Initialize(root_key, folder_key, 0);
}

RegistryKeyIterator::RegistryKeyIterator(HKEY root_key,
                                         const char16_t* folder_key,
                                         REGSAM wow64access) {
  Initialize(root_key, folder_key, wow64access);
}

RegistryKeyIterator::~RegistryKeyIterator() {
  if (key_)
    ::RegCloseKey(key_);
}

DWORD RegistryKeyIterator::SubkeyCount() const {
  DWORD count = 0;
  LONG result = ::RegQueryInfoKey(key_, NULL, 0, NULL, &count, NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL);
  if (result != ERROR_SUCCESS)
    return 0;

  return count;
}

bool RegistryKeyIterator::Valid() const {
  return key_ != NULL && index_ >= 0;
}

void RegistryKeyIterator::operator++() {
  --index_;
  Read();
}

bool RegistryKeyIterator::Read() {
  if (Valid()) {
    DWORD ncount = std::size(name_);
    FILETIME written;
    LONG r = ::RegEnumKeyEx(key_, index_, ToWCharT(name_), &ncount, NULL, NULL,
                            NULL, &written);
    if (ERROR_SUCCESS == r)
      return true;
  }

  name_[0] = '\0';
  return false;
}

void RegistryKeyIterator::Initialize(HKEY root_key,
                                     const char16_t* folder_key,
                                     REGSAM wow64access) {
  DCHECK_EQ(wow64access & ~kWow64AccessMask, static_cast<REGSAM>(0));
  LONG result = RegOpenKeyEx(root_key, ToWCharT(folder_key), 0,
                             KEY_READ | wow64access, &key_);
  if (result != ERROR_SUCCESS) {
    key_ = NULL;
  } else {
    DWORD count = 0;
    result = ::RegQueryInfoKey(key_, NULL, 0, NULL, &count, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL);

    if (result != ERROR_SUCCESS) {
      ::RegCloseKey(key_);
      key_ = NULL;
    } else {
      index_ = count - 1;
    }
  }

  Read();
}

}  // namespace win
}  // namespace base
