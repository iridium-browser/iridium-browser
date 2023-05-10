# This file is used to manage Vulkan dependencies for several repos. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# Avoids the need for a custom root variable.
use_relative_paths = True

vars = {
  'chromium_git': 'https://chromium.googlesource.com',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': '14e5a04e70057972eef8a40df422e30a3b70e4b5',

  # Current revision of spirv-cross, the Khronos SPIRV cross compiler.
  'spirv_cross_revision': '09e60d74f6a1c8deed2c02c998661b2ec7dd712e',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': 'a41cf67dfa52d33c049dc1b606e2db5cf72dafeb',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': '5f4e694e1022b7a6721ba14375b349e3dafcad4c',

  # Current revision of Khronos Vulkan-Headers.
  'vulkan_headers_revision': 'fa204df59c6caea6b9be3cf0754a88cd89056a87',

  # Current revision of Khronos Vulkan-Loader.
  'vulkan_loader_revision': '23c3a6047f64b584ab03797ec4875af9793a6f56',

  # Current revision of Khronos Vulkan-Tools.
  'vulkan_tools_revision': '728c1535dea8d04d4bd37308b3dbd3a0eff9726b',

  # Current revision of Khronos Vulkan-ValidationLayers.
  'vulkan_validation_revision': 'a83cf172869de8dd8dd2c90c1eceb11eee2db4e7',
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
