// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_FACED_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_FACED_DBUS_CONSTANTS_H_

namespace faced {

constexpr char kFaceAuthDaemonName[] = "org.chromium.FaceAuthDaemon";
constexpr char kFaceAuthDaemonPath[] = "/org/chromium/FaceAuthDaemon";
constexpr char kFaceAuthDaemonInterface[] = "org.chromium.FaceAuthDaemon";

// Methods
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";

// Token (pipe name) used to extract the message pipe from a
// Mojo invitation. This constant needs to be known on both ends of the
// pipe (Chromium and daemon).
constexpr char kBootstrapMojoConnectionChannelToken[] =
    "faceauth-service-bootstrap";

}  // namespace faced

#endif  // SYSTEM_API_DBUS_FACED_DBUS_CONSTANTS_H_
