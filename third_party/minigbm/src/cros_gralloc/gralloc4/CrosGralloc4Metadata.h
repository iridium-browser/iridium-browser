/*
 * Copyright 2022 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CROSGRALLOC4METADATA_H
#define CROSGRALLOC4METADATA_H

#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/common/Cta861_3.h>
#include <aidl/android/hardware/graphics/common/Dataspace.h>
#include <aidl/android/hardware/graphics/common/Smpte2086.h>

#define CROS_GRALLOC4_METADATA_MAX_NAME_SIZE 1024

/*
 * The metadata for cros_gralloc_buffer-s that should reside in a shared memory region
 * instead of directly in cros_gralloc_handle-s.
 *
 * Any metadata that is mutable must be stored in this shared memory region as
 * cros_gralloc_handle-s can not be tracked and updated across processes.
 */
struct CrosGralloc4Metadata {
    /*
     * Name is stored in the shared memory metadata to simplify cros_gralloc_handle
     * creation. This allows us to keep handles small while avoiding variable sized
     * handles.
     */
    char name[CROS_GRALLOC4_METADATA_MAX_NAME_SIZE];
    aidl::android::hardware::graphics::common::BlendMode blendMode;
    aidl::android::hardware::graphics::common::Dataspace dataspace;
    std::optional<aidl::android::hardware::graphics::common::Cta861_3> cta861_3;
    std::optional<aidl::android::hardware::graphics::common::Smpte2086> smpte2086;
};

#endif
