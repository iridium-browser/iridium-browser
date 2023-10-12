# Copyright (c) 2021, Alliance for Open Media. All rights reserved
#
# This source code is subject to the terms of the BSD 2 Clause License and the
# Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License was
# not distributed with this source code in the LICENSE file, you can obtain it
# at www.aomedia.org/license/software. If the Alliance for Open Media Patent
# License 1.0 was not distributed with this source code in the PATENTS file, you
# can obtain it at www.aomedia.org/license/patent.

cmake_minimum_required(VERSION 3.6)
project(avifinfo C CXX)
set(CMAKE_C_STANDARD 11)

option(AVIFINFO_BUILD_TESTS
       "Build and enable tests (GoogleTest must be installed)" OFF)
option(AVIFINFO_BUILD_TOOLS "Build tools" OFF)

# C library

add_library(avifinfo avifinfo.c)
target_include_directories(avifinfo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# C++ tests

if(AVIFINFO_BUILD_TESTS)
  find_package(Threads REQUIRED) # Needed by GoogleTest
  find_package(GTest REQUIRED)

  enable_testing()
  add_executable(avifinfo_test tests/avifinfo_test.cc)
  target_include_directories(avifinfo_test PRIVATE ${GTEST_INCLUDE_DIRS})
  target_link_libraries(avifinfo_test PRIVATE ${GTEST_BOTH_LIBRARIES} avifinfo)
  add_test(
    NAME avifinfo_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/avifinfo_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  add_executable(avifinfo_demo tests/avifinfo_demo.c)
  target_link_libraries(avifinfo_demo PRIVATE avifinfo)
  add_test(
    NAME avifinfo_demo
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/avifinfo_demo avifinfo_test_1x1.avif
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

# C++ tools

if(AVIFINFO_BUILD_TOOLS)
  find_package(Threads REQUIRED) # For aom

  # Clone and build libavif and its dependency aom.
  include(ExternalProject)
  set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)
  ExternalProject_Add(
    libavif
    GIT_REPOSITORY https://github.com/AOMediaCodec/libavif
    GIT_TAG main
    PATCH_COMMAND cd ${CMAKE_BINARY_DIR}/libavif-prefix/src/libavif/ext &&
                  ./aom.cmd
    UPDATE_DISCONNECTED 1 # Avoid building aom everytime.
    # aom.cmd builds aom as static so libavif should be built as static too.
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
               -DAVIF_CODEC_AOM=ON -DAVIF_LOCAL_AOM=ON -DBUILD_SHARED_LIBS=OFF)

  add_executable(avifinfo_tool tools/avifinfo_tool.cc tests/avifinfo_fuzz.cc)
  set_property(TARGET avifinfo_tool PROPERTY CXX_STANDARD 17) # for filesystem
  target_include_directories(avifinfo_tool SYSTEM
                             PRIVATE ${EXTERNAL_INSTALL_LOCATION}/include)
  # libavif's CMakeLists.txt uses a PRIVATE target_link_libraries() so
  # avifinfo_tool must be linked to aom too (and thus also to pthread).
  target_link_directories(
    avifinfo_tool PRIVATE ${EXTERNAL_INSTALL_LOCATION}/lib
    ${CMAKE_BINARY_DIR}/libavif-prefix/src/libavif/ext/aom/build.libavif)
  target_link_libraries(avifinfo_tool PRIVATE avifinfo avif aom
                                              Threads::Threads)
  add_dependencies(avifinfo_tool libavif)
endif()
