// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>
#include <vector>

#include "content_analysis/sdk/analysis_client.h"

using content_analysis::sdk::Client;
using content_analysis::sdk::ContentAnalysisRequest;
using content_analysis::sdk::ContentAnalysisResponse;
using content_analysis::sdk::ContentAnalysisAcknowledgement;

// Global app config.
bool user_specific = false;

// Paramters used to build the request.
content_analysis::sdk::AnalysisConnector connector =
    content_analysis::sdk::FILE_ATTACHED;
std::string request_token = "req-12345";
std::string tag = "dlp";
std::string digest = "sha256-123456";
std::string url = "https://upload.example.com";
std::vector<std::string> datas;

// Command line parameters.
constexpr const char* kArgConnector = "--connector=";
constexpr const char* kArgRequestToken = "--request-token=";
constexpr const char* kArgTag = "--tag=";
constexpr const char* kArgDigest = "--digest=";
constexpr const char* kArgUrl = "--url=";
constexpr const char* kArgUserSpecific = "--user";
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
      } else if (connector_str == "file-transfer") {
        connector = content_analysis::sdk::FILE_TRANSFER;
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
    } else if (arg.find(kArgUserSpecific) == 0) {
      user_specific = true;
    } else if (arg.find(kArgHelp) == 0) {
      return false;
    } else {
      datas.push_back(arg);
    }
  }

  return true;
}

void PrintHelp() {
  std::cout
    << std::endl << std::endl
    << "Usage: client [OPTIONS] [@]content_or_file ..." << std::endl
    << "A simple client to send content analysis requests to a running agent." << std::endl
    << "Without @ the content to analyze is the argument itself." << std::endl
    << "Otherwise the content is read from a file called 'content_or_file'." << std::endl
    << "Multiple [@]content_or_file arguments may be specified, each generates one request." << std::endl
    << std::endl << "Options:"  << std::endl
    << kArgConnector << "<connector> : one of 'download', 'attach' (default), 'bulk-data-entry', 'print', or 'file-transfer'" << std::endl
    << kArgRequestToken << "<unique-token> : defaults to 'req-12345'" << std::endl
    << kArgTag << "<tag> : defaults to 'dlp'" << std::endl
    << kArgUrl << "<url> : defaults to 'https://upload.example.com'" << std::endl
    << kArgUserSpecific << " : Connects to an OS user specific agent" << std::endl
    << kArgDigest << "<digest> : defaults to 'sha256-123456'" << std::endl
    << kArgHelp << " : prints this help message" << std::endl;
}

ContentAnalysisRequest BuildRequest(const std::string& data) {
  std::string filepath;
  std::string filename;
  if (data[0] == '@') {
    filepath = data.substr(1);
    filename = filepath.substr(filepath.find_last_of("/\\") + 1);
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

  if (!filepath.empty()) {
    request.set_file_path(filepath);
  } else if (!data.empty()) {
    request.set_text_content(data);
  } else {
    std::cout << "[Demo] Specify text content or a file path." << std::endl;
    PrintHelp();
    return ContentAnalysisRequest();
  }

  return request;
}

void DumpResponse(int position, const ContentAnalysisResponse& response) {
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
    default:
      status_str = "<Uknown>";
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

    std::cout << "[Demo] Request " << position << " is " << action_str
      << " after " << tag
      << " analysis, status=" << status_str << std::endl;
  }
}

ContentAnalysisAcknowledgement BuildAcknowledgement(
    const std::string& request_token) {
  ContentAnalysisAcknowledgement ack;
  ack.set_request_token(request_token);
  ack.set_status(ContentAnalysisAcknowledgement::SUCCESS);
  return ack;
}

int HandleRequest(Client* client,
                  int position,
                  const ContentAnalysisRequest& request) {
  ContentAnalysisResponse response;
  int err = client->Send(request, &response);
  if (err != 0) {
    std::cout << "[Demo] Error sending request " << position << std::endl;
    return 1;
  } else if (response.results_size() == 0) {
    std::cout << "[Demo] Response " << position << " is missing a result"
              << std::endl;
    return 1;
  } else {
    DumpResponse(position, response);

    int err = client->Acknowledge(
        BuildAcknowledgement(request.request_token()));
    if (err != 0) {
      std::cout << "[Demo] Error sending ack " << position << std::endl;
    }

    return 1;
  }

  return 0;
}

int main(int argc, char* argv[]) {
  if (!ParseCommandLine(argc, argv)) {
    PrintHelp();
    return 1;
  }

  // Each client uses a unique name to identify itself with Google Chrome.
  auto client = Client::Create({"content_analysis_sdk", user_specific});
  if (!client) {
    std::cout << "[Demo] Error starting client" << std::endl;
    return 1;
  };

  for (int i = 0; i < datas.size(); ++i) {
    HandleRequest(client.get(), i + 1, BuildRequest(datas[i]));
  }

  return 0;
};
