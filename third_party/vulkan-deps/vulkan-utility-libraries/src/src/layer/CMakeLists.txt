# Copyright 2023 The Khronos Group Inc.
# Copyright 2023 Valve Corporation
# Copyright 2023 LunarG, Inc.
#
# SPDX-License-Identifier: Apache-2.0
set(CMAKE_FOLDER "${CMAKE_FOLDER}/VulkanLayerSettings")

add_library(VulkanLayerSettings STATIC)
add_library(Vulkan::LayerSettings ALIAS VulkanLayerSettings)

target_compile_features(VulkanLayerSettings PRIVATE cxx_std_17)

target_sources(VulkanLayerSettings PRIVATE
   vk_layer_settings.cpp
   vk_layer_settings_helper.cpp
   layer_settings_manager.cpp
   layer_settings_manager.hpp
   layer_settings_util.cpp
   layer_settings_util.hpp
)

# NOTE: Because Vulkan::Headers header files are exposed in the public facing interface
# we must expose this library as public to users.
target_link_Libraries(VulkanLayerSettings PUBLIC Vulkan::Headers)

if(WIN32)
   target_compile_definitions(VulkanLayerSettings PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
