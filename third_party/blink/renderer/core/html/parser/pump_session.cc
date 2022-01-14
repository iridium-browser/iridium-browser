// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/parser/pump_session.h"

namespace blink {

PumpSession::PumpSession(unsigned& nesting_level)
    : NestingLevelIncrementer(nesting_level) {}

PumpSession::~PumpSession() = default;

}  // namespace blink
