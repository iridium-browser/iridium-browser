// Copyright 2023 The Khronos Group Inc.
// Copyright 2023 Valve Corporation
// Copyright 2023 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
// Author(s):
// - Christophe Riccio <christophe@lunarg.com>
#include <gtest/gtest.h>

#include "vulkan/layer/vk_layer_settings.hpp"
#include <iterator>
#include <vector>

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Bool) {
    const VkBool32 value_data{VK_TRUE};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    bool pValues = true;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);

    EXPECT_EQ(true, pValues);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_Bool) {
    const VkBool32 values_data[] = {VK_TRUE, VK_FALSE};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_BOOL32_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<bool> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);

    EXPECT_EQ(true, values[0]);
    EXPECT_EQ(false, values[1]);
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Int32) {
    const std::int32_t value_data{76};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::int32_t pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);

    EXPECT_EQ(76, pValues);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_Int32) {
    const std::int32_t values_data[] = {76, 82};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_INT32_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<std::int32_t> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);

    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(82, values[1]);
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Uint32) {
    const std::uint32_t value_data{76};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_UINT32_EXT, 1, &value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::uint32_t pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);

    EXPECT_EQ(76, pValues);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_Uint32) {
    const std::uint32_t values_data[] = {76, 82};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_UINT32_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<std::uint32_t> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);

    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(82, values[1]);
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Int64) {
    const std::int64_t value_data{76};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_INT64_EXT, 1, &value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::int64_t pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);

    EXPECT_EQ(76, pValues);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_Int64) {
    const std::int64_t values_data[] = {76, 82};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_INT64_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, &settings[0]};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<std::int64_t> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);

    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(82, values[1]);
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Uint64) {
    const std::uint64_t value_data{76};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_UINT64_EXT, 1, &value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::uint64_t pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);

    EXPECT_EQ(76, pValues);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_Uint64) {
    const std::uint64_t values_data[] = {76, 82};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_UINT64_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<std::uint64_t> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);

    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(82, values[1]);
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Float) {
    const float value_data{-82.5f};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_FLOAT32_EXT, 1, &value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    float pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);

    EXPECT_TRUE(std::abs(pValues - -82.5f) <= 0.0001f);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_Float) {
    const float values_data[] = {76.1f, -82.5f};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_FLOAT32_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<float> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);

    EXPECT_TRUE(std::abs(values[0] - 76.1f) <= 0.0001f);
    EXPECT_TRUE(std::abs(values[1] - -82.5f) <= 0.0001f);
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Double) {
    const double value_data{-82.5};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_FLOAT64_EXT, 1, &value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    double pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);

    EXPECT_TRUE(std::abs(pValues - -82.5) <= 0.0001);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_Double) {
    const double values_data[] = {76.1, -82.5};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_FLOAT64_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<double> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);

    EXPECT_TRUE(std::abs(values[0] - 76.1) <= 0.0001);
    EXPECT_TRUE(std::abs(values[1] - -82.5) <= 0.0001);
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_String) {
    const char* value_data[] = {"VALUE_A"};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_STRING_EXT, 1, value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    std::string pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);
    EXPECT_STREQ("VALUE_A", pValues.c_str());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Strings) {
    const char* values_data[] = {"VALUE_A", "VALUE_B"};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_STRING_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    std::string pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);
    EXPECT_STREQ("VALUE_A,VALUE_B", pValues.c_str());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_String) {
    const char* values_data[] = {"VALUE_A", "VALUE_B"};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_STRING_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<std::string> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);
    EXPECT_STREQ("VALUE_A", values[0].c_str());
    EXPECT_STREQ("VALUE_B", values[1].c_str());
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValue_Frameset) {
    const VlFrameset value_data{76, 100, 10};

    const VkLayerSettingEXT setting{"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_UINT32_EXT, 3, &value_data};

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &setting};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    VlFrameset pValues;
    vlGetLayerSettingValue(layerSettingSet, "my_setting", pValues);

    EXPECT_EQ(76, pValues.first);
    EXPECT_EQ(100, pValues.count);
    EXPECT_EQ(10, pValues.step);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_Frameset) {
    const VlFrameset values_data[] = {
        {76, 100, 10}, {1, 100, 1}
    };
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data) * (sizeof(VlFrameset) / sizeof(VlFrameset::count)));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_UINT32_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, settings};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<VlFrameset> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);

    EXPECT_EQ(76, values[0].first);
    EXPECT_EQ(100, values[0].count);
    EXPECT_EQ(10, values[0].step);
    EXPECT_EQ(1, values[1].first);
    EXPECT_EQ(100, values[1].count);
    EXPECT_EQ(1, values[1].step);
    EXPECT_EQ(2, values.size());

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_cpp, vlGetLayerSettingValues_VlCustomSTypeInfo) {
    const char* values_data[] = {"0x76", "0X82", "76", "82"};
    const uint32_t value_count = static_cast<uint32_t>(std::size(values_data));

    const VkLayerSettingEXT settings[] = {
        {"VK_LAYER_LUNARG_test", "my_setting", VK_LAYER_SETTING_TYPE_STRING_EXT, value_count, values_data}
    };
    const uint32_t settings_size = static_cast<uint32_t>(std::size(settings));

    const VkLayerSettingsCreateInfoEXT layer_settings_create_info{
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, settings_size, &settings[0]};

    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", &layer_settings_create_info, nullptr, nullptr, &layerSettingSet);

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    std::vector<VlCustomSTypeInfo> values;
    vlGetLayerSettingValues(layerSettingSet, "my_setting", values);
    EXPECT_EQ(0x76, values[0].first);
    EXPECT_EQ(0x82, values[0].second);
    EXPECT_EQ(76, values[1].first);
    EXPECT_EQ(82, values[1].second);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}
