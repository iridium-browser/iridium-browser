# Copyright 2023 The Khronos Group Inc.
# Copyright 2023 Valve Corporation
# Copyright 2023 LunarG, Inc.
#
# SPDX-License-Identifier: Apache-2.0
set(CMAKE_FOLDER "${CMAKE_FOLDER}/VulkanLayerSettings/tests")

find_package(GTest REQUIRED CONFIG)

include(GoogleTest)

# test_layer_setting_util
add_executable(test_layer_settings_util)

target_include_directories(test_layer_settings_util PRIVATE
    ${CMAKE_SOURCE_DIR}/src/layer
)

target_sources(test_layer_settings_util PRIVATE
    test_setting_util.cpp
)

target_link_libraries(test_layer_settings_util PRIVATE 
    GTest::gtest
    GTest::gtest_main
    Vulkan::Headers
    Vulkan::LayerSettings
)

gtest_discover_tests(test_layer_settings_util)

# test_layer_setting_api
add_executable(test_layer_settings_api)

target_include_directories(test_layer_settings_api PRIVATE
    ${CMAKE_SOURCE_DIR}/src/layer
)

target_sources(test_layer_settings_api PRIVATE
    test_setting_api.cpp
)

target_link_libraries(test_layer_settings_api PRIVATE 
    GTest::gtest
    GTest::gtest_main
    Vulkan::Headers
    Vulkan::LayerSettings
)

gtest_discover_tests(test_layer_settings_api)

# test_layer_setting_cpp
add_executable(test_layer_settings_cpp)

target_include_directories(test_layer_settings_cpp PRIVATE
    ${CMAKE_SOURCE_DIR}/src/layer
)

target_sources(test_layer_settings_cpp PRIVATE
    test_setting_cpp.cpp
)

target_link_libraries(test_layer_settings_cpp PRIVATE 
    GTest::gtest
    GTest::gtest_main
    Vulkan::Headers
    Vulkan::LayerSettings
)

gtest_discover_tests(test_layer_settings_cpp)

# test_layer_setting_env
add_executable(test_layer_settings_env)

target_include_directories(test_layer_settings_env PRIVATE
    ${CMAKE_SOURCE_DIR}/src/layer
)

target_sources(test_layer_settings_env PRIVATE
    test_setting_env.cpp
)

target_link_libraries(test_layer_settings_env PRIVATE 
    GTest::gtest
    GTest::gtest_main
    Vulkan::Headers
    Vulkan::LayerSettings
)

gtest_discover_tests(test_layer_settings_env)

# test_layer_setting_file
add_executable(test_layer_setting_file)

target_include_directories(test_layer_setting_file PRIVATE
    ${CMAKE_SOURCE_DIR}/src/layer
)

target_sources(test_layer_setting_file PRIVATE
    test_setting_file.cpp
)

target_link_libraries(test_layer_setting_file PRIVATE 
    GTest::gtest
    GTest::gtest_main
    Vulkan::Headers
    Vulkan::LayerSettings
)

gtest_discover_tests(test_layer_setting_file)

# test_layer_setting_cast
add_executable(test_layer_setting_cast)

target_include_directories(test_layer_setting_cast PRIVATE
    ${CMAKE_SOURCE_DIR}/src/layer
)

target_sources(test_layer_setting_cast PRIVATE
    test_setting_cast.cpp
)

target_link_libraries(test_layer_setting_cast PRIVATE 
    GTest::gtest
    GTest::gtest_main
    Vulkan::Headers
    Vulkan::LayerSettings
)

gtest_discover_tests(test_layer_setting_cast)
