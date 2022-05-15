/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define LOG_TAG "AllocatorService"

#include <hidl/LegacySupport.h>

#include "cros_gralloc/gralloc3/CrosGralloc3Allocator.h"

using android::sp;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::graphics::mapper::V3_0::Error;

int main(int, char**) {
    sp<CrosGralloc3Allocator> allocator = new CrosGralloc3Allocator();
    if (allocator->init() != Error::NONE) {
        ALOGE("Failed to initialize IAllocator 3.0 service.");
        return -EINVAL;
    }
    configureRpcThreadpool(4, true /* callerWillJoin */);
    if (allocator->registerAsService() != android::NO_ERROR) {
        ALOGE("Failed to register graphics IAllocator 3.0 service.");
        return -EINVAL;
    }

    ALOGI("IAllocator 3.0 service is initialized.");
    android::hardware::joinRpcThreadpool();
    ALOGI("IAllocator 3.0 service is terminating.");
    return 0;
}
