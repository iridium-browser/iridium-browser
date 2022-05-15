// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <iostream>
#include <string>

#include "content_analysis/sdk/analysis_agent.h"

using content_analysis::sdk::Agent;
using content_analysis::sdk::Session;
using content_analysis::sdk::ContentAnalysisRequest;
using content_analysis::sdk::ContentAnalysisResponse;

void DumpRequest(const ContentAnalysisRequest& request) {
  std::string connector = "<Unknown>";
  if (request.has_analysis_connector()) {
    switch (request.analysis_connector())
    {
    case content_analysis::sdk::FILE_DOWNLOADED:
      connector = "download";
      break;
    case content_analysis::sdk::FILE_ATTACHED:
      connector = "attach";
      break;
    case content_analysis::sdk::BULK_DATA_ENTRY:
      connector = "bulk-data-entry";
      break;
    case content_analysis::sdk::PRINT:
      connector = "print";
      break;
    default:
      break;
    }
  }

  std::string url =
      request.has_request_data() && request.request_data().has_url()
      ? request.request_data().url() : "<No URL>";

  std::string filename =
      request.has_request_data() && request.request_data().has_filename()
      ? request.request_data().filename() : "<No filename>";

  std::string digest =
      request.has_request_data() && request.request_data().has_digest()
      ? request.request_data().digest() : "<No digest>";

  std::string file_path =
      request.has_file_path()
      ? request.file_path() : "None, bulk text entry";

  std::cout << "Request: " << request.request_token() << std::endl;
  std::cout << "  Connector: " << connector << std::endl;
  std::cout << "  URL: " << url << std::endl;
  std::cout << "  Filename: " << filename << std::endl;
  std::cout << "  Digest: " << digest << std::endl;
  std::cout << "  Filepath: " << file_path << std::endl;
}

bool ReadContentFromFile(const std::string& file_path,
                         std::string* content) {
  std::ifstream file(file_path,
                     std::ios::in | std::ios::binary | std::ios::ate);
  if (!file.is_open())
    return false;

  // Get file size.  This example does not handle files larger than 1MB.
  // Make sure content string can hold the contents of the file.
  int size = file.tellg();
  if (size > 1024 * 1024)
    return false;

  content->resize(size + 1);

  // Read file into string.
  file.seekg(0, std::ios::beg);
  file.read(&(*content)[0], size);
  content->at(size) = 0;
  return true;
}

bool ShouldBlockRequest(const std::string& content) {
  // Determines if the request should be blocked.  For this simple example
  // the content is blocked if the string "block" is found.  Otherwise the
  // content is allowed.
  return content.find("block") != std::string::npos;
}

// Analyzes one request from Google Chrome and responds back to the browser
// with either an allow or block verdict.
void AnalyzeContent(std::unique_ptr<Session> session) {
  // A session represents one content analysis request and response triggered
  // by a user action in Google Chrome.  The agent determines whether the
  // user is allowed to perform the action by examining session->GetRequest().
  // The verdict, which can be "allow" or "block" is written into
  // session->GetResponse().

  std::cout << std::endl << "----------" << std::endl << std::endl;

  DumpRequest(session->GetRequest());

  bool block = false;
  bool success = true;

  if (session->GetRequest().has_text_content()) {
    block = ShouldBlockRequest(
        session->GetRequest().text_content());
  } else if (session->GetRequest().has_file_path()) {
    std::string content;
    success =
        ReadContentFromFile(session->GetRequest().file_path(),
                            &content);
    if (success) {
      block = ShouldBlockRequest(content);
    }
  }

  if (!success) {
    content_analysis::sdk::UpdateResponse(session->GetResponse(), std::string(),
        ContentAnalysisResponse::Result::FAILURE);
    std::cout << "  Verdict: failed to reach verdict" << std::endl;
  } else if (block) {
    content_analysis::sdk::SetSessionVerdictToBlock(session.get());
    std::cout << "  Verdict: block" << std::endl;
  } else {
    std::cout << "  Verdict: allow" << std::endl;
  }

  std::cout << std::endl;

  // Send the response back to Google Chrome.
  if (session->Send() == 0) {
    std::cout << "Acknowledgement:" << std::endl;
    if (session->GetAcknowledgement().verdict_received()) {
      std::cout << "  Verdict Received: true" << std::endl;
    } else {
      std::cout << "  Verdict Received: false" << std::endl;
    }
  } else {
    std::cout << "[Demo] Error sending response" << std::endl;
  }
}

int main(int argc, char* argv[]) {
  // Each agent uses a unique URI to identify itself with Google Chrome.
  auto agent = Agent::Create({"content_analysis_sdk"});
  if (!agent) {
    std::cout << "[Demo] Error starting agent" << std::endl;
    return 1;
  };

  while (true) {
    // Creates a new content analysis session, blocking until one is sent to
    // the agent from Google Chrome.
    auto session = agent->GetNextSession();
    if (!session) {
      std::cout << "[Demo] Error starting session" << std::endl;
      return 1;
    }

    // If the agent is capable of analyzing content in the background, the
    // sessions may be handled in background threads.  Having said that, a
    // session should not be assumed to be thread safe, that is, it should not
    // be accessed by more than one thread concurrently.
    //
    // In this example code, the session is handled synchronously.
    AnalyzeContent(std::move(session));
  }

  return 0;
};