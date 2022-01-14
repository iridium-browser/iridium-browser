// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_CONSTANTS_IMAGELOADER_H_
#define SYSTEM_API_CONSTANTS_IMAGELOADER_H_

namespace imageloader {

// The root path of all DLC module manifests.
const char kDlcManifestRootpath[] = "/opt/google/dlc/";

// The root path of all DLC module images.
const char kDlcImageRootpath[] = "/var/cache/dlc/";

}  // namespace imageloader

#endif  // SYSTEM_API_CONSTANTS_IMAGELOADER_H_
