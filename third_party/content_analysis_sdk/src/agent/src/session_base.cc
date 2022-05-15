// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "session_base.h"

namespace content_analysis {
namespace sdk {

int SessionBase::Close() {
  return 0;
}

int UpdateResponse(ContentAnalysisResponse& response,
                   const std::string& tag,
                   ContentAnalysisResponse::Result::Status status) {
  auto result = response.results_size() > 0
      ? response.mutable_results(0)
      : response.add_results();

  if (!tag.empty()) {
    result->set_tag(tag);
  }

  if (status != ContentAnalysisResponse::Result::STATUS_UNKNOWN) {
    result->set_status(status);
  }

  return 0;
}

int SetSessionVerdictTo(
    Session* session,
    ContentAnalysisResponse::Result::TriggeredRule::Action action) {
  // This function expects that the session's result has already been
  // initialized by a call to UpdateResponse().

  if (session->GetResponse().results_size() == 0) {
    return -1;
  }

  auto result = session->GetResponse().mutable_results(0);

  // Content analysis responses generated with this SDK contain at most one
  // triggered rule.
  auto rule = result->triggered_rules_size() > 0
      ? result->mutable_triggered_rules(0)
      : result->add_triggered_rules();

  rule->set_action(action);

  return 0;
}

int SetSessionVerdictToBlock(Session* session) {
  return SetSessionVerdictTo(session,
      ContentAnalysisResponse::Result::TriggeredRule::BLOCK);
}

}  // namespace sdk
}  // namespace content_analysis
