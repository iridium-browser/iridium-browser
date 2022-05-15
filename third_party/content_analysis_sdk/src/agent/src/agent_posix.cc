// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "agent_posix.h"
#include "session_posix.h"

namespace content_analysis {
namespace sdk {

// static
std::unique_ptr<Agent> Agent::Create(Config config) {
  return std::make_unique<AgentPosix>(std::move(config));
}

AgentPosix::AgentPosix(Config config) : AgentBase(std::move(config)) {}

std::unique_ptr<Session> AgentPosix::GetNextSession() {
  return std::make_unique<SessionPosix>();
}

}  // namespace sdk
}  // namespace content_analysis
