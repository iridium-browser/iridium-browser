// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_ANALYSIS_BROWSER_SRC_CLIENT_WIN_H_
#define CONTENT_ANALYSIS_BROWSER_SRC_CLIENT_WIN_H_

#include <string>

#include "client_base.h"

namespace content_analysis {
namespace sdk {

// Client implementaton for Windows.
class ClientWin : public ClientBase {
 public:
  ClientWin(Config config);

  // Client:
  int Send(const ContentAnalysisRequest& request,
                 ContentAnalysisResponse* response) override;

 private:
  DWORD ConnectToPipe(HANDLE* handle);

  std::string pipename_;
};

std::vector<char> ReadNextMessageFromPipe(HANDLE pipe);
bool WriteMessageToPipe(HANDLE pipe, const std::string& message);

}  // namespace sdk
}  // namespace content_analysis

#endif  // CONTENT_ANALYSIS_BROWSER_SRC_CLIENT_WIN_H_