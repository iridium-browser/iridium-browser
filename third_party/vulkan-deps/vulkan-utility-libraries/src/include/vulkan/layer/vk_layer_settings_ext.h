// Copyright 2023 The Khronos Group Inc.
// Copyright 2023 Valve Corporation
// Copyright 2023 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
// Author(s):
// - Christophe Riccio <christophe@lunarg.com>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif 

#include <vulkan/vk_layer.h>

#define VK_EXT_layer_settings 1
#define VK_EXT_LAYER_SETTINGS_SPEC_VERSION 1
#define VK_EXT_LAYER_SETTINGS_EXTENSION_NAME "VK_EXT_layer_settings"
//
// NOTE: VK_STRUCTURE_TYPE_MAX_ENUM - 1 is used by the intel driver.
// NOTE: VK_STRUCTURE_TYPE_MAX_ENUM - 42 is used by the validation layers
#define VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT ((VkStructureType)(VK_STRUCTURE_TYPE_MAX_ENUM - 44))

typedef enum VkLayerSettingTypeEXT {
    VK_LAYER_SETTING_TYPE_BOOL32_EXT = 0,
    VK_LAYER_SETTING_TYPE_INT32_EXT,
    VK_LAYER_SETTING_TYPE_INT64_EXT,
    VK_LAYER_SETTING_TYPE_UINT32_EXT,
    VK_LAYER_SETTING_TYPE_UINT64_EXT,
    VK_LAYER_SETTING_TYPE_FLOAT32_EXT,
    VK_LAYER_SETTING_TYPE_FLOAT64_EXT,
    VK_LAYER_SETTING_TYPE_STRING_EXT
} VkLayerSettingTypeEXT;

typedef struct VkLayerSettingEXT {
    const char *pLayerName;
    const char *pSettingName;
    VkLayerSettingTypeEXT type;
    uint32_t count;
    const void *pValues;
} VkLayerSettingEXT;

typedef struct VkLayerSettingsCreateInfoEXT {
    VkStructureType sType;
    const void *pNext;
    uint32_t settingCount;
    const VkLayerSettingEXT *pSettings;
} VkLayerSettingsCreateInfoEXT;

#ifdef __cplusplus
}
#endif
