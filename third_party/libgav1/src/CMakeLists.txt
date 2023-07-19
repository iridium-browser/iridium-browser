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

# libgav1 requires modern CMake.
cmake_minimum_required(VERSION 3.7.1 FATAL_ERROR)

# libgav1 requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(ABSL_CXX_STANDARD 11)
# libgav1 requires C99.
set(CMAKE_C_STANDARD 99)

project(libgav1 CXX C)

set(libgav1_root "${CMAKE_CURRENT_SOURCE_DIR}")
set(libgav1_build "${CMAKE_BINARY_DIR}")

if("${libgav1_root}" STREQUAL "${libgav1_build}")
  message(
    FATAL_ERROR
      "Building from within the libgav1 source tree is not supported.\n"
      "Hint: Run these commands\n" "$ rm -rf CMakeCache.txt CMakeFiles\n"
      "$ mkdir -p ../libgav1_build\n" "$ cd ../libgav1_build\n"
      "And re-run CMake from the libgav1_build directory.")
endif()

set(libgav1_examples "${libgav1_root}/examples")
set(libgav1_source "${libgav1_root}/src")

include("${libgav1_root}/cmake/libgav1_options.cmake")

libgav1_option(NAME LIBGAV1_ENABLE_OPTIMIZATIONS HELPSTRING
               "Enables optimized code." VALUE ON)
libgav1_option(NAME LIBGAV1_ENABLE_AVX2 HELPSTRING "Enables avx2 optimizations."
               VALUE ON)
libgav1_option(NAME LIBGAV1_ENABLE_NEON HELPSTRING "Enables neon optimizations."
               VALUE ON)
libgav1_option(NAME LIBGAV1_ENABLE_SSE4_1 HELPSTRING
               "Enables sse4.1 optimizations." VALUE ON)
libgav1_option(NAME LIBGAV1_ENABLE_EXAMPLES HELPSTRING "Enables examples." VALUE
               ON)
libgav1_option(NAME LIBGAV1_ENABLE_TESTS HELPSTRING "Enables tests." VALUE ON)
libgav1_option(
  NAME LIBGAV1_VERBOSE HELPSTRING
  "Enables verbose build system output. Higher numbers are more verbose." VALUE
  OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Enable generators like Xcode and Visual Studio to place projects in folders.
get_property(use_folders_is_set GLOBAL PROPERTY USE_FOLDERS SET)
if(NOT use_folders_is_set)
  set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)
endif()

include(FindThreads)

include("${libgav1_examples}/libgav1_examples.cmake")
include("${libgav1_root}/cmake/libgav1_build_definitions.cmake")
include("${libgav1_root}/cmake/libgav1_cpu_detection.cmake")
include("${libgav1_root}/cmake/libgav1_flags.cmake")
include("${libgav1_root}/cmake/libgav1_helpers.cmake")
include("${libgav1_root}/cmake/libgav1_install.cmake")
include("${libgav1_root}/cmake/libgav1_intrinsics.cmake")
include("${libgav1_root}/cmake/libgav1_sanitizer.cmake")
include("${libgav1_root}/cmake/libgav1_targets.cmake")
include("${libgav1_root}/cmake/libgav1_variables.cmake")
include("${libgav1_root}/tests/libgav1_tests.cmake")
include("${libgav1_source}/dsp/libgav1_dsp.cmake")
include("${libgav1_source}/libgav1_decoder.cmake")
include("${libgav1_source}/utils/libgav1_utils.cmake")

libgav1_optimization_detect()
libgav1_set_build_definitions()
libgav1_set_cxx_flags()
libgav1_configure_sanitizer()

# Supported bit depth.
libgav1_track_configuration_variable(LIBGAV1_MAX_BITDEPTH)

# C++ and linker flags.
libgav1_track_configuration_variable(LIBGAV1_CXX_FLAGS)
libgav1_track_configuration_variable(LIBGAV1_EXE_LINKER_FLAGS)

# Sanitizer integration.
libgav1_track_configuration_variable(LIBGAV1_SANITIZE)

# Generated source file directory.
libgav1_track_configuration_variable(LIBGAV1_GENERATED_SOURCES_DIRECTORY)

# Controls use of std::mutex and absl::Mutex in ThreadPool.
libgav1_track_configuration_variable(LIBGAV1_THREADPOOL_USE_STD_MUTEX)
if((DEFINED
    LIBGAV1_THREADPOOL_USE_STD_MUTEX
    AND NOT LIBGAV1_THREADPOOL_USE_STD_MUTEX)
   OR NOT (DEFINED LIBGAV1_THREADPOOL_USE_STD_MUTEX OR ANDROID OR IOS))
  set(use_absl_threading TRUE)
endif()

if(LIBGAV1_VERBOSE)
  libgav1_dump_cmake_flag_variables()
  libgav1_dump_tracked_configuration_variables()
  libgav1_dump_options()
endif()

set(libgav1_abseil_build "${libgav1_build}/abseil")
set(libgav1_gtest_build "${libgav1_build}/gtest")

# Compiler/linker flags must be lists, but come in from the environment as
# strings. Break them up:
if(NOT "${LIBGAV1_CXX_FLAGS}" STREQUAL "")
  separate_arguments(LIBGAV1_CXX_FLAGS)
endif()
if(NOT "${LIBGAV1_EXE_LINKER_FLAGS}" STREQUAL "")
  separate_arguments(LIBGAV1_EXE_LINKER_FLAGS)
endif()

# Set test-only flags based on LIBGAV1_CXX_FLAGS.
libgav1_set_test_flags()

set(libgav1_abseil "${libgav1_root}/third_party/abseil-cpp")
if(EXISTS "${libgav1_abseil}")
  set(ABSL_PROPAGATE_CXX_STD ON)
  add_subdirectory("${libgav1_abseil}" "${libgav1_abseil_build}"
                   EXCLUDE_FROM_ALL)
else()
  if(use_absl_threading OR LIBGAV1_ENABLE_EXAMPLES OR LIBGAV1_ENABLE_TESTS)
    message(
      FATAL_ERROR
        "Abseil not found. This dependency is required by the"
        " examples & tests and libgav1 when LIBGAV1_THREADPOOL_USE_STD_MUTEX is"
        " not defined. To continue, download the Abseil repository to"
        " third_party/abseil-cpp:\n  git \\\n    -C ${libgav1_root} \\\n"
        "    clone \\\n"
        "    https://github.com/abseil/abseil-cpp.git third_party/abseil-cpp")
  endif()
endif()

libgav1_reset_target_lists()
libgav1_add_dsp_targets()
libgav1_add_decoder_targets()
libgav1_add_examples_targets()
libgav1_add_tests_targets()
libgav1_add_utils_targets()
libgav1_setup_install_target()

if(LIBGAV1_ENABLE_TESTS)
  # include(CTest) or -DBUILD_TESTING=1 aren't used to avoid enabling abseil
  # tests.
  enable_testing()
endif()

if(LIBGAV1_VERBOSE)
  libgav1_dump_cmake_flag_variables()
  libgav1_dump_tracked_configuration_variables()
  libgav1_dump_options()
endif()
