# Copyright 2023 The Khronos Group Inc.
# Copyright 2023 Valve Corporation
# Copyright 2023 LunarG, Inc.
#
# SPDX-License-Identifier: Apache-2.0
target_include_directories(VulkanLayerSettings PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_sources(VulkanLayerSettings PRIVATE
	vulkan/layer/vk_layer_settings.h
	vulkan/layer/vk_layer_settings.hpp
	vulkan/layer/vk_layer_settings_ext.h
)

# https://cmake.org/cmake/help/latest/release/3.19.html#other
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
	target_sources(VulkanUtilityHeaders PRIVATE
		vulkan/utility/vul_dispatch_table.h
		vulkan/vk_enum_string_helper.h
	)
endif()

# NOTE: Because Vulkan::Headers header files are exposed in the public facing interface
# we must expose this library as public to users.
target_link_Libraries(VulkanUtilityHeaders INTERFACE Vulkan::Headers)

target_include_directories(VulkanUtilityHeaders INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
