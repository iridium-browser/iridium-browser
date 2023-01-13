// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_OCR_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_OCR_DBUS_CONSTANTS_H_

namespace ocr {

constexpr char kOcrServiceName[] = "org.chromium.OpticalCharacterRecognition";
constexpr char kOcrServiceInterface[] =
    "org.chromium.OpticalCharacterRecognition";
constexpr char kOcrServicePath[] = "/org/chromium/OpticalCharacterRecognition";

constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";

// Token (pipe name) used to extract the message pipe from a
// Mojo invitation. This constant needs to be known on both ends of the
// pipe (Chromium and daemon).
constexpr char kBootstrapMojoConnectionChannelToken[] = "ocr-service-bootstrap";

}  // namespace ocr

#endif  // SYSTEM_API_DBUS_OCR_DBUS_CONSTANTS_H_
