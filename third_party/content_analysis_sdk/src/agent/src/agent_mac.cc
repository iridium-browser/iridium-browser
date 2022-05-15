// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "agent_mac.h"
#include "session_mac.h"

namespace content_analysis {
namespace sdk {

// static
std::unique_ptr<Agent> Agent::Create(Config config) {
  return std::make_unique<AgentMac>(std::move(config));
}

AgentMac::AgentMac(Config config) : AgentBase(std::move(config)) {}

std::unique_ptr<Session> AgentMac::GetNextSession() {
  return std::make_unique<SessionMac>();
}

}  // namespace sdk
}  // namespace content_analysis
