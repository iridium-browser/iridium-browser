// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_FEATURED_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_FEATURED_DBUS_CONSTANTS_H_

namespace featured {

inline constexpr char kFeaturedInterface[] = "org.chromium.featured";
inline constexpr char kFeaturedServicePath[] = "/org/chromium/featured";
inline constexpr char kFeaturedServiceName[] = "org.chromium.featured";

// Methods.
inline constexpr char kHandleSeedFetchedMethod[] = "HandleSeedFetched";

}  // namespace featured

namespace feature {

inline constexpr char kFeatureLibInterface[] = "org.chromium.feature_lib";
inline constexpr char kFeatureLibServicePath[] = "/org/chromium/feature_lib";
inline constexpr char kFeatureLibServiceName[] = "org.chromium.feature_lib";

// Signals.
inline constexpr char kRefetchSignal[] = "RefetchFeatureState";
inline constexpr char kActiveTrialFileCreatedSignal[] =
    "ActiveTrialFileCreated";

}  // namespace feature

#endif  // SYSTEM_API_DBUS_FEATURED_DBUS_CONSTANTS_H_
