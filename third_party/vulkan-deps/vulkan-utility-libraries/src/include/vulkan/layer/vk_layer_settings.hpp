// Copyright 2023 The Khronos Group Inc.
// Copyright 2023 Valve Corporation
// Copyright 2023 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
// Author(s):
// - Christophe Riccio <christophe@lunarg.com>

#pragma once

#include "vk_layer_settings.h"
#include <vector>
#include <string>

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, bool &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector <bool> &settingValues);

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, int32_t &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector<int32_t> &settingValues);

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, int64_t &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector<int64_t> &settingValues);

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, uint32_t &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector<uint32_t> &settingValues);

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, uint64_t &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector<uint64_t> &settingValues);

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, float &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector<float> &settingValues);

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, double &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector<double> &settingValues);

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::string &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector<std::string> &settingValues);

void vlGetLayerSettingValue(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, VlFrameset &settingValue);

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName, std::vector<VlFrameset> &settingValues);

// Required by vk_safe_struct
typedef std::pair<uint32_t, uint32_t> VlCustomSTypeInfo;

void vlGetLayerSettingValues(VlLayerSettingSet layerSettingSet,
    const char *pSettingName,
    std::vector<VlCustomSTypeInfo> &settingValues);
