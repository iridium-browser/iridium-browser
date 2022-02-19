// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_COMMON_EXTENSIONS_API_CAST_API_PERMISSIONS_H_
#define CHROMECAST_COMMON_EXTENSIONS_API_CAST_API_PERMISSIONS_H_

#include <vector>

#include "base/containers/span.h"
#include "extensions/common/alias.h"
#include "extensions/common/permissions/api_permission.h"

namespace cast_api_permissions {

// Returns the information necessary to construct the APIPermissions usable in
// chromecast.
base::span<const extensions::APIPermissionInfo::InitInfo> GetPermissionInfos();

// Returns the list of aliases for extension APIPermissions usable in
// chromecast.
std::vector<extensions::Alias> GetPermissionAliases();

}  // namespace cast_api_permissions

#endif  // CHROMECAST_COMMON_EXTENSIONS_API_CAST_API_PERMISSIONS_H_
