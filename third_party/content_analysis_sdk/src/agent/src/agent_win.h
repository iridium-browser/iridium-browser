// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_ANALYSIS_AGENT_SRC_AGENT_WIN_H_
#define CONTENT_ANALYSIS_AGENT_SRC_AGENT_WIN_H_

#include <windows.h>

#include <string>

#include "agent_base.h"

namespace content_analysis {
namespace sdk {

// Agent implementaton for Windows.
class AgentWin : public AgentBase {
 public:
  AgentWin(Config config);
  ~AgentWin() override;

  // Agent:
  std::unique_ptr<Session> GetNextSession() override;
  int Stop() override;

 private:
  // Creates a new server endpoint of the pipe and returns the handle. If
  // successful ERROR_SUCCESS is returned and `handle` is set to the pipe's
  // server endpoint handle.  Otherwise an error code is returned and `handle`
  // is set to INVALID_HANDLE_VALUE.
  DWORD CreatePipe(HANDLE* handle);

  // Blocks and waits until a client connects.
  DWORD ConnectPipe(HANDLE handle);

  void Shutdown();

  std::string pipename_;
  HANDLE hPipe_ = INVALID_HANDLE_VALUE;
};

}  // namespace sdk
}  // namespace content_analysis

#endif  //q CONTENT_ANALYSIS_AGENT_SRC_AGENT_WIN_H_