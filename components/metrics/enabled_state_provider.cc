// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/enabled_state_provider.h"

#include "base/base_switches.h"
#include "base/command_line.h"

namespace metrics {

bool EnabledStateProvider::IsReportingEnabled() const {
  // Disable metrics reporting when field trials are forced, as otherwise this
  // would pollute experiment results in UMA. Note: This is done here and not
  // in IsConsentGiven() as we do not want this to affect field trial entropy
  // logic (i.e. high entropy source should still be used if UMA is on).
  return IsConsentGiven() && !base::CommandLine::ForCurrentProcess()->HasSwitch(
                                 switches::kForceFieldTrials);
}

}  // namespace metrics
