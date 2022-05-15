// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "content_analysis/sdk/analysis_client.h"

using content_analysis::sdk::Client;
using content_analysis::sdk::ContentAnalysisRequest;
using content_analysis::sdk::ContentAnalysisResponse;

// Paramters used to build the request.
content_analysis::sdk::AnalysisConnector connector =
    content_analysis::sdk::FILE_ATTACHED;
std::string request_token = "req-12345";
std::string tag = "dlp";
std::string digest = "sha256-123456";
std::string url = "https://upload.example.com";
std::string text_content;
std::string file_path;
std::string filename;

// Command line parameters.
constexpr const char* kArgConnector = "--connector=";
constexpr const char* kArgRequestToken = "--request-token=";
constexpr const char* kArgTag = "--tag=";
constexpr const char* kArgDigest = "--digest=";
constexpr const char* kArgUrl = "--url=";
constexpr const char* kArgHelp = "--help";

bool ParseCommandLine(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.find(kArgConnector) == 0) {
      std::string connector_str = arg.substr(strlen(kArgConnector));
      if (connector_str == "download") {
        connector = content_analysis::sdk::FILE_DOWNLOADED;
      } else if (connector_str == "attach") {
        connector = content_analysis::sdk::FILE_ATTACHED;
      } else if (connector_str == "bulk-data-entry") {
        connector = content_analysis::sdk::BULK_DATA_ENTRY;
      } else if (connector_str == "print") {
        connector = content_analysis::sdk::PRINT;
      } else {
        std::cout << "[Demo] Incorrect command line arg: " << arg << std::endl;
        return false;
      }
    } else if (arg.find(kArgRequestToken) == 0) {
      request_token = arg.substr(strlen(kArgRequestToken));
    } else if (arg.find(kArgTag) == 0) {
      tag = arg.substr(strlen(kArgTag));
    } else if (arg.find(kArgDigest) == 0) {
      digest = arg.substr(strlen(kArgDigest));
    } else if (arg.find(kArgUrl) == 0) {
      url = arg.substr(strlen(kArgUrl));
    } else if (arg.find(kArgHelp) == 0) {
      return false;
    } else {
      if (arg[0] == '@') {
        file_path = arg.substr(1);
        filename = file_path.substr(file_path.find_last_of("/\\") + 1);
      } else {
        text_content = arg;
      }

      break;
    }
  }

  return true;
}

void PrintHelp() {
  std::cout
    << std::endl << std::endl
    << "Usage: client [OPTIONS] [@]content_or_file" << std::endl
    << "A simple client to send content analysis requests to a running agent." << std::endl
    << "Without @ the content to analyze is the argument itself." << std::endl
    << "Otherwise the content is read from a file called 'content_or_file'." << std::endl
    << std::endl << "Options:"  << std::endl
    << kArgConnector << "<connector> : one of 'download', 'attach' (default), 'bulk-data-entry', or 'print'" << std::endl
    << kArgRequestToken << "<unique-token> : defaults to 'req-12345'" << std::endl
    << kArgTag << "<tag> : defaults to 'dlp'" << std::endl
    << kArgUrl << "<url> : defaults to 'https://upload.example.com'" << std::endl
    << kArgDigest << "<digest> : defaults to 'sha256-123456'" << std::endl
    << kArgHelp << " : prints this help message" << std::endl;
}

int main(int argc, char* argv[]) {
  // Each client uses a unique URI to identify itself with Google Chrome.
  auto client = Client::Create({"content_analysis_sdk"});
  if (!client) {
    std::cout << "[Demo] Error starting client" << std::endl;
    return 1;
  };

  if (!ParseCommandLine(argc, argv)) {
    PrintHelp();
    return 1;
  }

  ContentAnalysisRequest request;
  request.set_analysis_connector(connector);
  request.set_request_token(request_token);
  *request.add_tags() = tag;

  auto request_data = request.mutable_request_data();
  request_data->set_url(url);
  request_data->set_digest(digest);
  if (!filename.empty()) {
    request_data->set_filename(filename);
  }

  if (!text_content.empty()) {
    request.set_text_content(text_content);
  } else if (!file_path.empty()) {
    request.set_file_path(file_path);
  } else {
    std::cout << "[Demo] Specify text content or a file path." << std::endl;
    PrintHelp();
    return 1;
  }

  ContentAnalysisResponse response;
  int err = client->Send(request, &response);
  if (err != 0) {
    std::cout << "[Demo] Error sending request" << std::endl;
    return 1;
  }

  // Print whether the request should be blocked or not.
  if (response.results_size() == 0) {
    std::cout << "[Demo] Response is missing a result" << std::endl;
  } else {
    for (auto result : response.results()) {
      auto tag = result.has_tag() ? result.tag() : "<no-tag>";

      auto status = result.has_status()
          ? result.status()
          : ContentAnalysisResponse::Result::STATUS_UNKNOWN;
      std::string status_str;
      switch (status) {
      case ContentAnalysisResponse::Result::STATUS_UNKNOWN:
        status_str = "Unknown";
        break;
      case ContentAnalysisResponse::Result::SUCCESS:
        status_str = "Success";
        break;
      case ContentAnalysisResponse::Result::FAILURE:
        status_str = "Failure";
        break;
      }

      auto action =
          ContentAnalysisResponse::Result::TriggeredRule::ACTION_UNSPECIFIED;
      for (auto rule : result.triggered_rules()) {
        if (rule.has_action() && rule.action() > action)
          action = rule.action();
      }
      std::string action_str;
      switch (action) {
        case ContentAnalysisResponse::Result::TriggeredRule::ACTION_UNSPECIFIED:
          action_str = "allowed";
          break;
        case ContentAnalysisResponse::Result::TriggeredRule::REPORT_ONLY:
          action_str = "reported only";
          break;
        case ContentAnalysisResponse::Result::TriggeredRule::WARN:
          action_str = "warned";
          break;
        case ContentAnalysisResponse::Result::TriggeredRule::BLOCK:
          action_str = "blocked";
          break;
      }

      std::cout << "[Demo] Request is " << action_str
                << " after " << tag
                << " analysis, status=" << status_str << std::endl;
    }
  }

  return 0;
};