# Copyright 2023 The Khronos Group Inc.
# Copyright 2023 Valve Corporation
# Copyright 2023 LunarG, Inc.
#
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.17)

project(TEST_ADD_SUBDIRECTORY LANGUAGES C)

add_library(add_subdirectory_example STATIC)

# Test c99 support
target_compile_features(add_subdirectory_example PRIVATE c_std_99)

target_sources(add_subdirectory_example PRIVATE
    client.c
    vk_enum_string_helper.c
    vul_dispatch_table.c
)

# NOTE: Because VulkanHeaders is a PUBLIC dependency it needs to be found prior to VulkanUtilityLibraries
add_subdirectory(${GITHUB_VULKAN_HEADER_SOURCE_DIR} ${PROJECT_BINARY_DIR}/headers)

add_subdirectory(${PROJECT_SOURCE_DIR}/../../ ${PROJECT_BINARY_DIR}/vul)

if (NOT TARGET Vulkan::LayerSettings)
    message(FATAL_ERROR "Vulkan::LayerSettings target not defined!")
endif()

if (NOT TARGET Vulkan::UtilityHeaders)
    message(FATAL_ERROR "Vulkan::UtilityHeaders target not defined!")
endif()

target_link_libraries(add_subdirectory_example PRIVATE Vulkan::LayerSettings Vulkan::UtilityHeaders)
