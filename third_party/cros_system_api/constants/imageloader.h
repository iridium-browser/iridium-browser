// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_CONSTANTS_IMAGELOADER_H_
#define SYSTEM_API_CONSTANTS_IMAGELOADER_H_

namespace imageloader {

// The root path of all DLC module manifests.
const char kDlcManifestRootpath[] = "/opt/google/dlc/";
const char kRelativeDlcManifestRootpath[] = "opt/google/dlc/";

// The root path of all DLC module images.
const char kDlcImageRootpath[] = "/var/cache/dlc/";

// The default path where all imageloader managed images get mounted.
const char kImageloaderMountBase[] = "/run/imageloader/";

}  // namespace imageloader

#endif  // SYSTEM_API_CONSTANTS_IMAGELOADER_H_
