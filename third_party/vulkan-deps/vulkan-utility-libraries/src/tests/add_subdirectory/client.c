// Copyright 2023 The Khronos Group Inc.
// Copyright 2023 Valve Corporation
// Copyright 2023 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
#include <vulkan/layer/vk_layer_settings.h>

VkBool32 foobar() {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", NULL, NULL, NULL, &layerSettingSet);

    VkBool32 result = vlHasLayerSetting(layerSettingSet, "setting_key") ? VK_TRUE : VK_FALSE;

    vlDestroyLayerSettingSet(layerSettingSet, NULL);

    return result;
}
