// Copyright 2023 The Khronos Group Inc.
// Copyright 2023 Valve Corporation
// Copyright 2023 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
#include <vulkan/utility/vul_dispatch_table.h>

PFN_vkVoidFunction local_vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
    (void)instance;
    (void)pName;
    return NULL;
}

PFN_vkVoidFunction local_vkGetDeviceProcAddr(VkDevice device, const char *pName) {
    (void)device;
    (void)pName;
    return NULL;
}

void foobar() {
    VulDeviceDispatchTable device_dispatch_table;
    VulInstanceDispatchTable instance_dispatch_table;

    VkInstance instance = VK_NULL_HANDLE;

    vulInitInstanceDispatchTable(instance, &instance_dispatch_table, local_vkGetInstanceProcAddr);

    VkDevice device = VK_NULL_HANDLE;

    vulInitDeviceDispatchTable(device, &device_dispatch_table, local_vkGetDeviceProcAddr);
}
