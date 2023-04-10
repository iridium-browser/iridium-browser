// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_DCAD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_DCAD_DBUS_CONSTANTS_H_

namespace dcad {

constexpr char kDcaDaemonName[] = "org.chromium.DcaDaemon";
constexpr char kDcaDaemonPath[] = "/org/chromium/DcaDaemon";
constexpr char kDcaDaemonInterface[] = "org.chromium.DcaDaemon";

// Methods
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";

// Token (pipe name) used to extract the message pipe from a
// Mojo invitation. This constant needs to be known on both ends of the
// pipe (Chromium and daemon).
constexpr char kBootstrapMojoConnectionChannelToken[] = "dca-daemon-bootstrap";

}  // namespace dcad

#endif  // SYSTEM_API_DBUS_DCAD_DBUS_CONSTANTS_H_
