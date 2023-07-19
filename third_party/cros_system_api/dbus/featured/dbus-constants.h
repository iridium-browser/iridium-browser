// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_FEATURED_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_FEATURED_DBUS_CONSTANTS_H_

namespace featured {

const char kFeaturedInterface[] = "org.chromium.featured";
const char kFeaturedServicePath[] = "/org/chromium/featured";
const char kFeaturedServiceName[] = "org.chromium.featured";

// Methods.
constexpr char kHandleSeedFetchedMethod[] = "HandleSeedFetched";

}  // namespace featured

#endif  // SYSTEM_API_DBUS_FEATURED_DBUS_CONSTANTS_H_
