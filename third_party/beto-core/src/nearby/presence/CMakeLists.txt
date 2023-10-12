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

project(NearbyProtocol)

# required for designated initializers on MSVC
set(CMAKE_CXX_STANDARD 17)

# root directory of repo
set(BETO_CORE_ROOT ${CMAKE_SOURCE_DIR}/../..)

# location of external third_party dependencies
set(THIRD_PARTY_DIR ${BETO_CORE_ROOT}/third_party)

set(CMAKE_C_FLAGS_DEBUG "-DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")
if (UNIX)
    set(CMAKE_C_FLAGS_DEBUG "-g ${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "-g ${CMAKE_C_FLAGS_DEBUG}")
endif ()

if (UNIX)
    add_compile_options(-Wall -Wextra -Wimplicit-fallthrough -Wextra-semi
            -Wno-missing-field-initializers -Wno-unused-parameter -Wno-psabi
            -Wshadow
            -Wsign-compare)
elseif (MSVC)
    add_compile_options(-W4 -O1)
endif ()

find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    message(STATUS "OpenSSL Found: ${OPENSSL_VERSION}")
    message(STATUS "OpenSSL Include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")
endif ()

if (ENABLE_TESTS)
    message(STATUS "Enabling workspace wide tests")

    # Setup GoogleTest
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    include(GoogleTest)

    # Find GoogleBenchmark
    find_package(benchmark REQUIRED)

    # Setup jsoncpp
    set(JSONCPP_DIR ${THIRD_PARTY_DIR}/jsoncpp)
    include_directories(${JSONCPP_DIR})
    add_library(
            jsoncpp
            ${JSONCPP_DIR}/jsoncpp.cpp
    )
endif ()

add_subdirectory(np_cpp_ffi)
add_subdirectory(ldt_np_c_sample)

if (ENABLE_FUZZ)
    message(STATUS "Building fuzzers")
    add_subdirectory(ldt_np_adv_ffi_fuzz)
endif ()
