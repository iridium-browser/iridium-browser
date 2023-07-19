# Copyright 2022 Google LLC
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

cmake_minimum_required(VERSION 3.14)

enable_testing()

include_directories(
    ${CMAKE_SOURCE_DIR}/ukey2_c_ffi/cpp/)

include(ExternalProject)
set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/target/tmp)
ExternalProject_Add(
    ukey2_c_ffi
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build COMMAND cargo build --release --lib
    BINARY_DIR "${CMAKE_SOURCE_DIR}/ukey2_c_ffi"
    INSTALL_COMMAND "")

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Werror -Wextra -Wimplicit-fallthrough -Wextra-semi
        -Wno-missing-field-initializers -Wno-unused-parameter -Wno-psabi
        -Wno-unneeded-internal-declaration
        -Wno-ignored-pragma-optimize
        -Wno-bitfield-constant-conversion -Wno-deprecated-this-capture -Wshadow
        -Wsign-compare)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

add_executable(ffi_test
  ukey2_test.cc
  ukey2_glue.cc
  ukey2_ffi.h
  ukey2_bindings.h)

target_link_libraries(ffi_test
    "${CMAKE_SOURCE_DIR}/../../../../target/release/libukey2_c_ffi${CMAKE_SHARED_LIBRARY_SUFFIX}"
    GTest::gtest_main
    dl pthread)

include(GoogleTest)
gtest_discover_tests(ffi_test)
