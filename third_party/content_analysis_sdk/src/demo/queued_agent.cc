// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <thread>

#include "content_analysis/sdk/analysis_agent.h"
#include "demo/handler.h"
#include "demo/request_queue.h"

// An AgentEventHandler that dumps requests information to stdout and blocks
// any requests that have the keyword "block" in their data
class QueuingHandler : public Handler {
 public:
  QueuingHandler() {
    StartBackgroundThread();
  }

  ~QueuingHandler() override {
    // Abort background process and wait for it to finish.
    request_queue_.abort();
    WaitForBackgroundThread();
  }

 private:
  void OnAnalysisRequested(std::unique_ptr<Event> event) override {
    request_queue_.push(std::move(event));
  }

  static void* ProcessRequests(void* qh) {
    QueuingHandler* handler = reinterpret_cast<QueuingHandler*>(qh);

    while (true) {
      auto event = handler->request_queue_.pop();
      if (!event)
        break;

      handler->AnalyzeContent(std::move(event));
    }

    return 0;
  }

  // A list of outstanding content analysis requests.
  RequestQueue request_queue_;

  void StartBackgroundThread() {
    thread_ = std::make_unique<std::thread>(ProcessRequests, this);
  }

  void WaitForBackgroundThread() {
    thread_->join();
  }

  // Thread id of backgrond thread.
  std::unique_ptr<std::thread> thread_;
};

int main(int argc, char* argv[]) {
  // Each agent uses a unique name to identify itself with Google Chrome.
  content_analysis::sdk::ResultCode rc;
  auto agent = content_analysis::sdk::Agent::Create({"content_analysis_sdk"},
      std::make_unique<QueuingHandler>(), &rc);
  if (!agent || rc != content_analysis::sdk::ResultCode::OK) {
    std::cout << "[Demo] Error starting agent: "
              << content_analysis::sdk::ResultCodeToString(rc)
              << std::endl;
    return 1;
  };

  // Blocks, sending events to the handler until agent->Stop() is called.
  rc = agent->HandleEvents();
  if (rc != content_analysis::sdk::ResultCode::OK) {
    std::cout << "[Demo] Error from handling events: "
              << content_analysis::sdk::ResultCodeToString(rc)
              << std::endl;
  }

  return 0;
};

