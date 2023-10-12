# This file is used to manage Vulkan dependencies for several repos. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# Avoids the need for a custom root variable.
use_relative_paths = True
git_dependencies = 'SYNC'

vars = {
  'chromium_git': 'https://chromium.googlesource.com',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': 'e3a711b6fc942307be7cb634698015dfd13c37fe',

  # Current revision of spirv-cross, the Khronos SPIRV cross compiler.
  'spirv_cross_revision': '54997fb4bc3adeb47b9b9f7bb67f1c25eaca2204',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': 'd790ced752b5bfc06b6988baadef6eb2d16bdf96',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': '4c16c35b16bbd462a0e89707ebeecc0bce956b2f',

  # Current revision of Khronos Vulkan-Headers.
  'vulkan_headers_revision': '2634c969d7dc0e983f005f7f2e665cce8449efe6',

  # Current revision of Khronos Vulkan-Loader.
  'vulkan_loader_revision': 'd34bfafff23602e857064bea6d99a35eb63f37f2',

  # Current revision of Khronos Vulkan-Tools.
  'vulkan_tools_revision': '389110e4600669d82bca042859fddf898387c0d2',

  # Current revision of Khronos Vulkan-Utility-Libraries.
  'vulkan_utility_libraries_revision': 'dd26ae7065a9ed60c8ab517ed79bf7935394aec7',

  # Current revision of Khronos Vulkan-ValidationLayers.
  'vulkan_validation_revision': 'b89f48ae89ec064c91e8b77468cebc18c3faa682',
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

  'vulkan-utility-libraries/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Utility-Libraries@{vulkan_utility_libraries_revision}',
  },

  'vulkan-validation-layers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-ValidationLayers@{vulkan_validation_revision}',
  },
}
