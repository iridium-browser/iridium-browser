// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_CONSTANTS_FEATURED_H_
#define SYSTEM_API_CONSTANTS_FEATURED_H_

namespace feature {

// Path where featured writes early-boot active trial files. Chrome reads these
// files and records active trials to UMA.
inline constexpr char kActiveTrialFileDirectory[] = "/run/featured/active";

// Character used to separate the trial name and group name in active trial
// filenames. This character is not allowed in trial names so it makes for a
// good separator.
//
// Context: FeatureList::IsValidFeatureOrFieldTrialName().
//
// NOTE: If a trial name contains "," because it was not validated, the
// separator will be escaped when writing the active trial file.
inline constexpr char kTrialGroupSeparator[] = ",";

}  // namespace feature

#endif  // SYSTEM_API_CONSTANTS_FEATURED_H_
