// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_ANALYSIS_AGENT_SRC_SESSION_WIN_H_
#define CONTENT_ANALYSIS_AGENT_SRC_SESSION_WIN_H_

#include <windows.h>

#include "session_base.h"

namespace content_analysis {
namespace sdk {

// Session implementaton for Windows.
class SessionWin : public SessionBase {
 public:
  SessionWin(HANDLE handle);
  ~SessionWin() override;

  // Initialize the session.  This involves reading the request from Google
  // Chrome and making sure it is well formed.
  DWORD Init();

  // Session:
  int Close() override;
  int Send() override;

 private:
  void Shutdown();

  HANDLE hPipe_ = INVALID_HANDLE_VALUE;
};

std::vector<char> ReadNextMessageFromPipe(HANDLE pipe);
bool WriteMessageToPipe(HANDLE pipe, const std::string& message);

}  // namespace sdk
}  // namespace content_analysis

#endif  // CONTENT_ANALYSIS_AGENT_SRC_SESSION_WIN_H_