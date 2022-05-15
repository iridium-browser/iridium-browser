// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include <windows.h>
#include <sddl.h>

#include "utils_win.h"

namespace content_analysis {
namespace sdk {
namespace internal {

std::string GetUserSID() {
  std::string sid;

  HANDLE handle;
  if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &handle) &&
    !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &handle)) {
    return std::string();
  }

  DWORD length = 0;
  std::vector<char> buffer;
  if (!GetTokenInformation(handle, TokenUser, nullptr, 0, &length)) {
    DWORD err = GetLastError();
    if (err == ERROR_INSUFFICIENT_BUFFER) {
      buffer.resize(length);
    }
  }
  if (GetTokenInformation(handle, TokenUser, buffer.data(), buffer.size(),
    &length)) {
    TOKEN_USER* info = reinterpret_cast<TOKEN_USER*>(buffer.data());
    char* sid_string;
    if (ConvertSidToStringSidA(info->User.Sid, &sid_string)) {
      sid = sid_string;
      LocalFree(sid_string);
    }
  }

  CloseHandle(handle);
  return sid;
}

std::string GetPipeName(const std::string& base, bool user_specific) {
  std::string pipename = "\\\\.\\pipe\\" + base;
  if (user_specific) {
    std::string sid = GetUserSID();
    if (sid.empty())
      return std::string();

    pipename += "." + sid;
  }

  return pipename;
}

}  // internal
}  // namespace sdk
}  // namespace content_analysis
