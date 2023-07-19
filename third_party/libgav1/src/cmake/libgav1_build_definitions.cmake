# Copyright 2019 The libgav1 Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(LIBGAV1_CMAKE_LIBGAV1_BUILD_DEFINITIONS_CMAKE_)
  return()
endif() # LIBGAV1_CMAKE_LIBGAV1_BUILD_DEFINITIONS_CMAKE_
set(LIBGAV1_CMAKE_LIBGAV1_BUILD_DEFINITIONS_CMAKE_ 1)

macro(libgav1_set_build_definitions)
  string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lowercase)

  libgav1_load_version_info()

  # Library version info. See the libtool docs for updating the values:
  # https://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
  #
  # c=<current>, r=<revision>, a=<age>
  #
  # libtool generates a .so file as .so.[c-a].a.r, while -version-info c:r:a is
  # passed to libtool.
  #
  # We set LIBGAV1_SOVERSION = [c-a].a.r
  set(LT_CURRENT 0)
  set(LT_REVISION 1)
  set(LT_AGE 0)
  math(EXPR LIBGAV1_SOVERSION_MAJOR "${LT_CURRENT} - ${LT_AGE}")
  set(LIBGAV1_SOVERSION "${LIBGAV1_SOVERSION_MAJOR}.${LT_AGE}.${LT_REVISION}")
  unset(LT_CURRENT)
  unset(LT_REVISION)
  unset(LT_AGE)

  list(APPEND libgav1_include_paths "${libgav1_root}" "${libgav1_root}/src"
              "${libgav1_build}" "${libgav1_root}/third_party/abseil-cpp")
  list(APPEND libgav1_gtest_include_paths
              "third_party/googletest/googlemock/include"
              "third_party/googletest/googletest/include"
              "third_party/googletest/googletest")
  list(APPEND libgav1_test_include_paths ${libgav1_include_paths}
              ${libgav1_gtest_include_paths})
  list(APPEND libgav1_defines "LIBGAV1_CMAKE=1"
              "LIBGAV1_FLAGS_SRCDIR=\"${libgav1_root}\""
              "LIBGAV1_FLAGS_TMPDIR=\"/tmp\"")

  if(MSVC OR WIN32)
    list(APPEND libgav1_defines "_CRT_SECURE_NO_WARNINGS" "NOMINMAX"
                "_SCL_SECURE_NO_WARNINGS")
  endif()

  if(ANDROID)
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
      set(CMAKE_ANDROID_ARM_MODE ON)
    endif()

    if(build_type_lowercase MATCHES "rel")
      list(APPEND libgav1_base_cxx_flags "-fno-stack-protector")
    endif()
  endif()

  list(APPEND libgav1_base_cxx_flags "-Wall" "-Wextra" "-Wmissing-declarations"
              "-Wno-sign-compare" "-fvisibility=hidden"
              "-fvisibility-inlines-hidden")

  if(BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(libgav1_dependency libgav1_shared)
  else()
    set(libgav1_dependency libgav1_static)
  endif()

  list(APPEND libgav1_clang_cxx_flags "-Wextra-semi" "-Wmissing-prototypes"
              "-Wshorten-64-to-32")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6")
      # Quiet warnings in copy-list-initialization where {} elision has always
      # been allowed.
      list(APPEND libgav1_clang_cxx_flags "-Wno-missing-braces")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8)
      list(APPEND libgav1_clang_cxx_flags "-Wextra-semi-stmt")
    endif()
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "7")
      # Quiet warnings due to potential snprintf() truncation in threadpool.cc.
      list(APPEND libgav1_base_cxx_flags "-Wno-format-truncation")

      if(CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7")
        # Quiet gcc 6 vs 7 abi warnings:
        # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77728
        list(APPEND libgav1_base_cxx_flags "-Wno-psabi")
        list(APPEND ABSL_GCC_FLAGS "-Wno-psabi")
      endif()
    endif()
  endif()

  if(build_type_lowercase MATCHES "rel")
    list(APPEND libgav1_base_cxx_flags "-Wframe-larger-than=196608")
  endif()

  list(APPEND libgav1_msvc_cxx_flags
              # Warning level 3.
              "/W3"
              # Disable warning C4018:
              # '<comparison operator>' signed/unsigned mismatch
              "/wd4018"
              # Disable warning C4244:
              # 'argument': conversion from '<double/int>' to
              # '<float/smaller int type>', possible loss of data
              "/wd4244"
              # Disable warning C4267:
              # '=': conversion from '<double/int>' to
              # '<float/smaller int type>', possible loss of data
              "/wd4267"
              # Disable warning C4309:
              # 'argument': truncation of constant value
              "/wd4309"
              # Disable warning C4551:
              # function call missing argument list
              "/wd4551")

  if(BUILD_SHARED_LIBS)
    list(APPEND libgav1_msvc_cxx_flags
                # Disable warning C4251:
                # 'libgav1::DecoderImpl class member' needs to have
                # dll-interface to be used by clients of class
                # 'libgav1::Decoder'.
                "/wd4251")
  endif()

  if(NOT LIBGAV1_MAX_BITDEPTH)
    set(LIBGAV1_MAX_BITDEPTH 10)
  elseif(NOT LIBGAV1_MAX_BITDEPTH EQUAL 8
         AND NOT LIBGAV1_MAX_BITDEPTH EQUAL 10
         AND NOT LIBGAV1_MAX_BITDEPTH EQUAL 12)
    libgav1_die("LIBGAV1_MAX_BITDEPTH must be 8, 10 or 12.")
  endif()

  list(APPEND libgav1_defines "LIBGAV1_MAX_BITDEPTH=${LIBGAV1_MAX_BITDEPTH}")

  if(DEFINED LIBGAV1_THREADPOOL_USE_STD_MUTEX)
    if(NOT LIBGAV1_THREADPOOL_USE_STD_MUTEX EQUAL 0
       AND NOT LIBGAV1_THREADPOOL_USE_STD_MUTEX EQUAL 1)
      libgav1_die("LIBGAV1_THREADPOOL_USE_STD_MUTEX must be 0 or 1.")
    endif()

    list(APPEND libgav1_defines
         "LIBGAV1_THREADPOOL_USE_STD_MUTEX=${LIBGAV1_THREADPOOL_USE_STD_MUTEX}")
  endif()

  # Source file names ending in these suffixes will have the appropriate
  # compiler flags added to their compile commands to enable intrinsics.
  set(libgav1_avx2_source_file_suffix "avx2(_test)?.cc")
  set(libgav1_neon_source_file_suffix "neon(_test)?.cc")
  set(libgav1_sse4_source_file_suffix "sse4(_test)?.cc")
endmacro()
