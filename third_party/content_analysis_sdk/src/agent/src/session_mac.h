// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_ANALYSIS_SRC_SESSION_MAC_H_
#define CONTENT_ANALYSIS_SRC_SESSION_MAC_H_

#include "session_base.h"

namespace content_analysis {
namespace sdk {

// Session implementaton for macos.
class SessionMac : public SessionBase {
 public:
  SessionMac() = default;

  // Session:
  int Send() override;

  // TODO(rogerta): Fill in implementation.
};

}  // namespace sdk
}  // namespace content_analysis

#endif  // CONTENT_ANALYSIS_SRC_SESSION_MAC_H_
