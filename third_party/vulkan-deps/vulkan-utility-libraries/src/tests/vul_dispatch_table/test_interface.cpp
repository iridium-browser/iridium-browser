// Copyright 2023 The Khronos Group Inc.
// Copyright 2023 Valve Corporation
// Copyright 2023 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <gtest/gtest.h>

#include <vulkan/utility/vul_dispatch_table.h>

// Only exists so that local_vkGetDeviceProcAddr can return a 'real' function pointer
inline void empty_func() {}

inline PFN_vkVoidFunction local_vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
    if (instance == VK_NULL_HANDLE) return NULL;

    if (strcmp(pName, "vkGetInstanceProcAddr")) return reinterpret_cast<PFN_vkVoidFunction>(&local_vkGetInstanceProcAddr);

    return reinterpret_cast<PFN_vkVoidFunction>(&empty_func);
}

inline PFN_vkVoidFunction local_vkGetDeviceProcAddr(VkDevice device, const char *pName) {
    if (device == VK_NULL_HANDLE) return NULL;

    if (strcmp(pName, "vkGetDeviceProcAddr")) return reinterpret_cast<PFN_vkVoidFunction>(&local_vkGetDeviceProcAddr);
    return reinterpret_cast<PFN_vkVoidFunction>(&empty_func);
}

TEST(test_vul_dispatch_table, cpp_interface) {
    VulDeviceDispatchTable device_dispatch_table{};
    VulInstanceDispatchTable instance_dispatch_table{};

    VkInstance instance{};

    vulInitInstanceDispatchTable(instance, &instance_dispatch_table, local_vkGetInstanceProcAddr);

    ASSERT_EQ(instance_dispatch_table.GetInstanceProcAddr, local_vkGetInstanceProcAddr);

    VkDevice device{};

    vulInitDeviceDispatchTable(device, &device_dispatch_table, local_vkGetDeviceProcAddr);

    ASSERT_EQ(device_dispatch_table.GetDeviceProcAddr, local_vkGetDeviceProcAddr);
}
