# This file is used to manage Vulkan dependencies for several repos. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# Avoids the need for a custom root variable.
use_relative_paths = True

vars = {
  'chromium_git': 'https://chromium.googlesource.com',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': '5755de46b07e4374c05fb1081f65f7ae1f8cca81',

  # Current revision of spirv-cross, the Khronos SPIRV cross compiler.
  'spirv_cross_revision': 'f09ba2777714871bddb70d049878af34b94fa54d',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': '85a1ed200d50660786c1a88d9166e871123cce39',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': 'b53d3a6be38b032dedbc72639dfc6249b5e92697',

  # Current revision of Khronos Vulkan-Headers.
  'vulkan_headers_revision': '98f440ce6868c94f5ec6e198cc1adda4760e8849',

  # Current revision of Khronos Vulkan-Loader.
  'vulkan_loader_revision': '61187c40845c92a80bcd2216089dd01badcda7ac',

  # Current revision of Khronos Vulkan-Tools.
  'vulkan_tools_revision': 'e52fa1cf2d95503d28f9d020800cbab15aaa304b',

  # Current revision of Khronos Vulkan-ValidationLayers.
  'vulkan_validation_revision': 'ada4a476f4dfadda94f21ca80d35f7a0cd37cba9',
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
