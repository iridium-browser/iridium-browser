# This file is used to manage Vulkan dependencies for several repos. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# Avoids the need for a custom root variable.
use_relative_paths = True

vars = {
  'chromium_git': 'https://chromium.googlesource.com',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': '6a7ec4be7b8a22ab16cea0f294b5973dbcdd637a',

  # Current revision of spirv-cross, the Khronos SPIRV cross compiler.
  'spirv_cross_revision': '2d3a152081ca6e6bea7093940d0f81088fe4d01c',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': '6e09e44cd88a5297433411b2ee52f4cf9f50fa90',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': 'a63ac9f73d29cd27cdb6e3388d98d1d934e512bb',

  # Current revision of Khronos Vulkan-Headers.
  'vulkan_headers_revision': 'c1a8560c5cf5e7bd6dbc71fe69b1a317411c36b8',

  # Current revision of Khronos Vulkan-Loader.
  'vulkan_loader_revision': 'db51885950c860961279168997b5cde12a77abf9',

  # Current revision of Khronos Vulkan-Tools.
  'vulkan_tools_revision': '0cab6e8055fb0f3a54d8314552fd523a3da57c2c',

  # Current revision of Khronos Vulkan-ValidationLayers.
  'vulkan_validation_revision': 'e2be2287f4820ed578d1adeca981736146a74d9a',
}

deps = {
  'glslang/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/glslang@{glslang_revision}',
  },

  'spirv-cross/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Cross@{spirv_cross_revision}',
  },

  'spirv-headers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Headers@{spirv_headers_revision}',
  },

  'spirv-tools/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Tools@{spirv_tools_revision}',
  },

  'vulkan-headers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Headers@{vulkan_headers_revision}',
  },

  'vulkan-loader/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Loader@{vulkan_loader_revision}',
  },

  'vulkan-tools/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Tools@{vulkan_tools_revision}',
  },

  'vulkan-validation-layers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-ValidationLayers@{vulkan_validation_revision}',
  },
}
