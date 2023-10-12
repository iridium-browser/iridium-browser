// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SHADERCACHED_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SHADERCACHED_DBUS_CONSTANTS_H_

namespace shadercached {

constexpr char kShaderCacheInterface[] = "org.chromium.ShaderCache";
constexpr char kShaderCacheServicePath[] = "/org/chromium/ShaderCache";
constexpr char kShaderCacheServiceName[] = "org.chromium.ShaderCache";

// Methods
constexpr char kInstallMethod[] = "Install";
constexpr char kUninstallMethod[] = "Uninstall";
constexpr char kPurgeMethod[] = "Purge";
constexpr char kUnmountMethod[] = "Unmount";
constexpr char kPrepareShaderCache[] = "PrepareShaderCache";

// Signals
constexpr char kShaderCacheMountStatusChanged[] =
    "ShaderCacheMountStatusChanged";

}  // namespace shadercached

#endif  // SYSTEM_API_DBUS_SHADERCACHED_DBUS_CONSTANTS_H_
