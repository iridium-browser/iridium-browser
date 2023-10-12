// Copyright 2023 The Khronos Group Inc.
// Copyright 2023 Valve Corporation
// Copyright 2023 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
// Author(s):
// - Christophe Riccio <christophe@lunarg.com>
#include <gtest/gtest.h>

#include "vulkan/layer/vk_layer_settings.h"
#include <vector>

void test_helper_SetLayerSetting(VlLayerSettingSet layerSettingSet, const char *pSettingName, const char *pValue);

TEST(test_layer_setting_file, vlGetLayerSettingValues_Bool) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "true,false");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 0;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_BOOL32, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<VkBool32> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_BOOL32, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_EQ(VK_TRUE, values[0]);
    EXPECT_EQ(VK_FALSE, values[1]);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_BOOL32, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_EQ(VK_TRUE, values[0]);
    EXPECT_EQ(VK_FALSE, values[1]);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_file, vlGetLayerSettingValues_Int32) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "76,-82");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 2;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_INT32, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<std::int32_t> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_INT32, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(0, values[1]);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_INT32, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(-82, values[1]);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_file, vlGetLayerSettingValues_Int64) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "76,-82");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 2;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_INT64, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<std::int64_t> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_INT64, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(0, values[1]);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_INT64, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(-82, values[1]);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_file, vlGetLayerSettingValues_Uint32) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "76,82");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 0;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_UINT32, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<std::uint32_t> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_UINT32, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(0, values[1]);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_UINT32, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(82, values[1]);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_file, vlGetLayerSettingValues_Uint64) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "76,82");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 0;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_UINT64, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<std::uint64_t> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_UINT64, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(0, values[1]);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_UINT64, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_EQ(76, values[0]);
    EXPECT_EQ(82, values[1]);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_file, vlGetLayerSettingValues_Float) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "76.1f,-82.5f");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 0;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FLOAT32, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<float> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FLOAT32, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_TRUE(std::abs(values[0] - 76.1f) <= 0.0001f);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FLOAT32, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_TRUE(std::abs(values[0] - 76.1f) <= 0.0001f);
    EXPECT_TRUE(std::abs(values[1] - -82.5f) <= 0.0001f);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_file, vlGetLayerSettingValues_Double) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "76.1,-82.5");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 0;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FLOAT64, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<double> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FLOAT64, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_TRUE(std::abs(values[0] - 76.1) <= 0.0001);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FLOAT64, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_TRUE(std::abs(values[0] - 76.1) <= 0.0001);
    EXPECT_TRUE(std::abs(values[1] - -82.5) <= 0.0001);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_file, vlGetLayerSettingValues_Frameset) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "76-100-10,1-100-1");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 0;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FRAMESET, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<VlFrameset> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FRAMESET, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_EQ(76, values[0].first);
    EXPECT_EQ(100, values[0].count);
    EXPECT_EQ(10, values[0].step);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_FRAMESET, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_EQ(76, values[0].first);
    EXPECT_EQ(100, values[0].count);
    EXPECT_EQ(10, values[0].step);
    EXPECT_EQ(1, values[1].first);
    EXPECT_EQ(100, values[1].count);
    EXPECT_EQ(1, values[1].step);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}

TEST(test_layer_setting_file, vlGetLayerSettingValues_String) {
    VlLayerSettingSet layerSettingSet = VK_NULL_HANDLE;
    vlCreateLayerSettingSet("VK_LAYER_LUNARG_test", nullptr, nullptr, nullptr, &layerSettingSet);

    test_helper_SetLayerSetting(layerSettingSet, "lunarg_test.my_setting", "VALUE_A,VALUE_B");

    EXPECT_TRUE(vlHasLayerSetting(layerSettingSet, "my_setting"));

    uint32_t value_count = 0;
    VkResult result_count =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_STRING, &value_count, nullptr);
    EXPECT_EQ(VK_SUCCESS, result_count);
    EXPECT_EQ(2, value_count);

    std::vector<const char*> values(static_cast<uint32_t>(value_count));

    value_count = 1;
    VkResult result_incomplete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_STRING, &value_count, &values[0]);
    EXPECT_EQ(VK_INCOMPLETE, result_incomplete);
    EXPECT_STREQ("VALUE_A", values[0]);
    EXPECT_STREQ(nullptr, values[1]);
    EXPECT_EQ(1, value_count);

    value_count = 2;
    VkResult result_complete =
        vlGetLayerSettingValues(layerSettingSet, "my_setting", VL_LAYER_SETTING_TYPE_STRING, &value_count, &values[0]);
    EXPECT_EQ(VK_SUCCESS, result_complete);
    EXPECT_STREQ("VALUE_A", values[0]);
    EXPECT_STREQ("VALUE_B", values[1]);
    EXPECT_EQ(2, value_count);

    vlDestroyLayerSettingSet(layerSettingSet, nullptr);
}
