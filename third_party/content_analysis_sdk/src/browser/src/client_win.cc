// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <utility>
#include <vector>

#include "common/utils_win.h"

#include "client_win.h"

namespace content_analysis {
namespace sdk {

const DWORD kBufferSize = 4096;

// static
std::unique_ptr<Client> Client::Create(Config config) {
  int rc;
  auto client = std::make_unique<ClientWin>(std::move(config), &rc);
  return rc == 0 ? std::move(client) : nullptr;
}

ClientWin::ClientWin(Config config, int* rc) : ClientBase(std::move(config)) {
  *rc = -1;

  std::string pipename =
    internal::GetPipeName(configuration().name, configuration().user_specific);
  if (pipename.empty()) {
    return;
  }

  pipename_ = pipename;
  if (ConnectToPipe(pipename_, &hPipe_) != ERROR_SUCCESS) {
    Shutdown();
  } else {
    *rc = 0;
  }
}

ClientWin::~ClientWin() {
  Shutdown();
}

int ClientWin::Send(const ContentAnalysisRequest& request,
                    ContentAnalysisResponse* response) {
  // TODO: could avoid a copy by changing first argument to be
  // `ContentAnalysisRequest request` and then using std::move() below and at
  // call site.
  ChromeToAgent chrome_to_agent;
  *chrome_to_agent.mutable_request() = request;
  bool success = WriteMessageToPipe(hPipe_,
                                    chrome_to_agent.SerializeAsString());
  if (success) {
    std::vector<char> buffer = ReadNextMessageFromPipe(hPipe_);
    AgentToChrome agent_to_chrome;
    success = agent_to_chrome.ParseFromArray(buffer.data(), buffer.size());
    if (success) {
      *response = std::move(*agent_to_chrome.mutable_response());
    }
  }

  return success ? 0 : -1;
}

int ClientWin::Acknowledge(const ContentAnalysisAcknowledgement& ack) {
  // TODO: could avoid a copy by changing argument to be
  // `ContentAnalysisAcknowledgement ack` and then using std::move() below and
  // at call site.
  ChromeToAgent chrome_to_agent;
  *chrome_to_agent.mutable_ack() = ack;
  return WriteMessageToPipe(hPipe_, chrome_to_agent.SerializeAsString())
      ? 0 : -1;
}

int ClientWin::CancelRequests(const ContentAnalysisCancelRequests& cancel) {
  // TODO: could avoid a copy by changing argument to be
  // `ContentAnalysisCancelRequests cancel` and then using std::move() below and
  // at call site.
  ChromeToAgent chrome_to_agent;
  *chrome_to_agent.mutable_cancel() = cancel;
  return WriteMessageToPipe(hPipe_, chrome_to_agent.SerializeAsString())
      ? 0 : -1;
}

// static
DWORD ClientWin::ConnectToPipe(const std::string& pipename, HANDLE* handle) {
  HANDLE h = INVALID_HANDLE_VALUE;
  while (h == INVALID_HANDLE_VALUE) {
    h = CreateFileA(pipename.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    /*shareMode=*/0,
                    /*securityAttr=*/nullptr, OPEN_EXISTING,
                    /*flags=*/0,
                    /*template=*/nullptr);
    if (h == INVALID_HANDLE_VALUE) {
      if (GetLastError() != ERROR_PIPE_BUSY) {
        break;
      }

      if (!WaitNamedPipeA(pipename.c_str(), NMPWAIT_USE_DEFAULT_WAIT)) {
        break;
      }
    }
  }

  if (h == INVALID_HANDLE_VALUE) {
    return GetLastError();
  }

  // Change to message read mode to match server side.  Max connection count
  // and timeout must be null if client and server are on the same machine.
  DWORD mode = PIPE_READMODE_MESSAGE;
  if (!SetNamedPipeHandleState(h, &mode,
                               /*maxCollectionCount=*/nullptr,
                               /*connectionTimeout=*/nullptr)) {
    DWORD err = GetLastError();
    CloseHandle(h);
    return err;
  }

  *handle = h;
  return ERROR_SUCCESS;
}

// static
std::vector<char> ClientWin::ReadNextMessageFromPipe(HANDLE pipe) {
  DWORD err = ERROR_SUCCESS;
  std::vector<char> buffer(kBufferSize);
  char* p = buffer.data();
  int final_size = 0;
  while (true) {
    DWORD read;
    if (ReadFile(pipe, p, kBufferSize, &read, nullptr)) {
      final_size += read;
      break;
    } else {
      err = GetLastError();
      if (err != ERROR_MORE_DATA)
        break;

      buffer.resize(buffer.size() + kBufferSize);
      p = buffer.data() + buffer.size() - kBufferSize;
    }
  }
  buffer.resize(final_size);
  return buffer;
}

// static
bool ClientWin::WriteMessageToPipe(HANDLE pipe, const std::string& message) {
  if (message.empty())
    return false;
  DWORD written;
  return WriteFile(pipe, message.data(), message.size(), &written, nullptr);
}

void ClientWin::Shutdown() {
  if (hPipe_ != INVALID_HANDLE_VALUE) {
    FlushFileBuffers(hPipe_);
    CloseHandle(hPipe_);
    hPipe_ = INVALID_HANDLE_VALUE;
  }
}

}  // namespace sdk
}  // namespace content_analysis