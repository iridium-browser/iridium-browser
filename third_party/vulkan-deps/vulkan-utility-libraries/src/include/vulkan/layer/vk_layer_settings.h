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

#include "vk_layer_settings_ext.h"

typedef enum VlLayerSettingType {
    VL_LAYER_SETTING_TYPE_BOOL32 = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
    VL_LAYER_SETTING_TYPE_INT32 = VK_LAYER_SETTING_TYPE_INT32_EXT,
    VL_LAYER_SETTING_TYPE_INT64 = VK_LAYER_SETTING_TYPE_INT64_EXT,
    VL_LAYER_SETTING_TYPE_UINT32 = VK_LAYER_SETTING_TYPE_UINT32_EXT,
    VL_LAYER_SETTING_TYPE_UINT64 = VK_LAYER_SETTING_TYPE_UINT64_EXT,
    VL_LAYER_SETTING_TYPE_FLOAT32 = VK_LAYER_SETTING_TYPE_FLOAT32_EXT,
    VL_LAYER_SETTING_TYPE_FLOAT64 = VK_LAYER_SETTING_TYPE_FLOAT64_EXT,
    VL_LAYER_SETTING_TYPE_STRING = VK_LAYER_SETTING_TYPE_STRING_EXT,
    VL_LAYER_SETTING_TYPE_FRAMESET,
    VL_LAYER_SETTING_TYPE_FRAMESET_STRING
} VlLayerSettingType;

VK_DEFINE_HANDLE(VlLayerSettingSet)

// - `first` is an integer related to the first frame to be processed.
//    The frame numbering is 0-based.
//  - `count` is an integer related to the number of frames to be
//    processed. A count of zero represents every frame after the start of the range.
//  - `step` is an integer related to the interval between frames. A step of zero
//    represent no frame to be processed.
//    between frames to be processed.

typedef struct VlFrameset {
    uint32_t first;
    uint32_t count;
    uint32_t step;
} VlFrameset;

typedef void (VKAPI_PTR *VlLayerSettingLogCallback)(const char *pSettingName, const char *pMessage);

// Create a layer setting set. If 'pCallback' is set to NULL, the messages are outputed to stderr.
VkResult vlCreateLayerSettingSet(const char *pLayerName, const VkLayerSettingsCreateInfoEXT *pCreateInfo,
                             const VkAllocationCallbacks *pAllocator, VlLayerSettingLogCallback pCallback,
                                 VlLayerSettingSet *pLayerSettingSet);

void vlDestroyLayerSettingSet(VlLayerSettingSet layerSettingSet, const VkAllocationCallbacks *pAllocator);

// Check whether a setting was set either programmatically, from vk_layer_settings.txt or an environment variable
VkBool32 vlHasLayerSetting(VlLayerSettingSet layerSettingSet, const char *pSettingName);

// Query setting values
VkResult vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet, const char *pSettingName, VlLayerSettingType type,
                                 uint32_t *pValueCount, void *pValues);

const VkLayerSettingsCreateInfoEXT *vlFindLayerSettingsCreateInfo(const VkInstanceCreateInfo *pCreateInfo);

#ifdef __cplusplus
}
#endif
