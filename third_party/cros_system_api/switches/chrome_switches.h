// Copyright 2013 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_SWITCHES_CHROME_SWITCHES_H_
#define SYSTEM_API_SWITCHES_CHROME_SWITCHES_H_

// This file defines switches that are used both by Chrome and login_manager.

namespace chromeos {
namespace switches {

// Sentinel switches for policy injected flags.
const char kPolicySwitchesBegin[] = "policy-switches-begin";
const char kPolicySwitchesEnd[] = "policy-switches-end";

// Flag passed to the browser if the system is running in dev-mode.
const char kSystemInDevMode[] = "system-developer-mode";

// Used to pass JSON-encoded list of strings specifying enabled feature flags.
// The format of the entries is the same that Chrome uses for persisting
// chrome://flags configuration, i.e. the "internal name" of the feature flag,
// optionally followed by an '@' and the index of the selected choice (for
// multi-value items). Example: ["dark-light-mode@1","tint-composited-content"]
const char kFeatureFlags[] = "feature-flags";

// Passes JSON-encoded dictionary of origin list flags (i.e. feature flags that
// take an optional string argument which is supposed to specify a list of web
// origins). Example: {"isolate-origins":"http://example.com"}
const char kFeatureFlagsOriginList[] = "feature-flags-origin-list";

}  // namespace switches
}  // namespace chromeos

#endif  // SYSTEM_API_SWITCHES_CHROME_SWITCHES_H_
