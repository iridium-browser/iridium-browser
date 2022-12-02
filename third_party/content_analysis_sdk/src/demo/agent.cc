// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <iostream>
#include <string>

#include "content_analysis/sdk/analysis_agent.h"
#include "demo/handler.h"

// Different paths are used depending on whether this agent should run as a
// use specific agent or not.  These values are chosen to match the test
// values in chrome browser.
constexpr char kPathUser[] = "path_user";
constexpr char kPathSystem[] = "path_system";

// Global app config.
const char* path = kPathSystem;
bool user_specific = false;

// Command line parameters.
constexpr const char* kArgUserSpecific = "--user";
constexpr const char* kArgHelp = "--help";

bool ParseCommandLine(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.find(kArgUserSpecific) == 0) {
      path = kPathUser;
      user_specific = true;
    } else if (arg.find(kArgHelp) == 0) {
      return false;
    }
  }

  return true;
}

void PrintHelp() {
  std::cout
    << std::endl << std::endl
    << "Usage: agent [OPTIONS]" << std::endl
    << "A simple agent to process content analysis requests." << std::endl
    << "Data containing the string 'block' blocks the request data from being used." << std::endl
    << std::endl << "Options:"  << std::endl
    << kArgUserSpecific << " : Make agent OS user specific" << std::endl
    << kArgHelp << " : prints this help message" << std::endl;
}

int main(int argc, char* argv[]) {
  if (!ParseCommandLine(argc, argv)) {
    PrintHelp();
    return 1;
  }

  // Each agent uses a unique name to identify itself with Google Chrome.
  content_analysis::sdk::ResultCode rc;
  auto agent = content_analysis::sdk::Agent::Create(
      {path, user_specific}, std::make_unique<Handler>(), &rc);
  if (!agent || rc != content_analysis::sdk::ResultCode::OK) {
    std::cout << "[Demo] Error starting agent: "
              << content_analysis::sdk::ResultCodeToString(rc)
              << std::endl;
    return 1;
  };

  std::cout << "[Demo] " << agent->DebugString() << std::endl;

  // Blocks, sending events to the handler until agent->Stop() is called.
  rc = agent->HandleEvents();
  if (rc != content_analysis::sdk::ResultCode::OK) {
    std::cout << "[Demo] Error from handling events: "
              << content_analysis::sdk::ResultCodeToString(rc)
              << std::endl;
    std::cout << "[Demo] " << agent->DebugString() << std::endl;
  }

  return 0;
}
