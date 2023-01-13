// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SYSTEM_API_DBUS_SYSTEM_PROXY_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SYSTEM_PROXY_DBUS_CONSTANTS_H_

namespace system_proxy {
// General
const char kSystemProxyInterface[] = "org.chromium.SystemProxy";
const char kSystemProxyServicePath[] = "/org/chromium/SystemProxy";
const char kSystemProxyServiceName[] = "org.chromium.SystemProxy";

// Methods
// Clears the user proxy auth credentials from System-proxy. This method is
// invoked when the user clears the credentials cache from the Browser.
const char kClearUserCredentialsMethod[] = "ClearUserCredentials";
// Sets authentication details which may contain credentials with the option to
// restrict them to a protection space or/and Kerberos auth details which allows
// System-proxy to request the Kerberos config and credentials cache files.
const char kSetAuthenticationDetailsMethod[] = "SetAuthenticationDetails";
// TODO(acostinas, crbug.com/1076377) Remove deprecated
// SetSystemTrafficCredentials call. Please use SetAuthenticationDetails.
const char kSetSystemTrafficCredentialsMethod[] = "SetSystemTrafficCredentials";
// Shuts down the System-proxy service or just one of the worker processes,
// depending on the argument.
const char kShutDownProcessMethod[] = "ShutDownProcess";
// Signals that a worker process is active.
const char kWorkerActiveSignal[] = "WorkerActive";
// Signals that System-proxy requires credentials to perform proxy
// authentication.
const char kAuthenticationRequiredSignal[] = "AuthenticationRequired";
}  // namespace system_proxy
#endif  // SYSTEM_API_DBUS_SYSTEM_PROXY_DBUS_CONSTANTS_H_
