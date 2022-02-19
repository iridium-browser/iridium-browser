# This file is used to manage Vulkan dependencies for several repos. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# Avoids the need for a custom root variable.
use_relative_paths = True

vars = {
  'chromium_git': 'https://chromium.googlesource.com',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': '542ee69d8309fda81ee5661c5e3cc88c2b9194fe',

  # Current revision of spirv-cross, the Khronos SPIRV cross compiler.
  'spirv_cross_revision': 'e9cc6403341baf0edd430a4027b074d0a06b782f',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': 'd53b49635b7484e86959608a65a64d8121e6a385',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': '438096e0c2524d8733aea6e9d56dc47921eedc79',

  # Current revision of Khronos Vulkan-Headers.
  'vulkan_headers_revision': 'e005e1f8175d006adc3676b40ac3dd2212961a68',

  # Current revision of Khronos Vulkan-Loader.
  'vulkan_loader_revision': '2bbfe2ef640cff1130bff423edca2b8c13baefc5',

  # Current revision of Khronos Vulkan-Tools.
  'vulkan_tools_revision': '08f87babadc491b12f34fe6a8692a97bbabb4b2d',

  # Current revision of Khronos Vulkan-ValidationLayers.
  'vulkan_validation_revision': 'ee8a454f88a89e8b71e6e7973faab4c76a3ac8eb',
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
