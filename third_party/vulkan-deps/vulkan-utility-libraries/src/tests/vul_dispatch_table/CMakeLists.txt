# Copyright 2023 The Khronos Group Inc.
# Copyright 2023 Valve Corporation
# Copyright 2023 LunarG, Inc.
#
# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED CONFIG)

include(GoogleTest)

add_executable(test_vul_dispatch_table test_interface.cpp)

target_link_libraries(test_vul_dispatch_table PRIVATE
    GTest::gtest
    GTest::gtest_main
    Vulkan::UtilityHeaders
)

if(${CMAKE_C_COMPILER_ID} MATCHES "(GNU|Clang)")
    add_compile_options(-Wpedantic -Wall -Wextra -Werror)
endif()


target_include_directories(test_vul_dispatch_table PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

gtest_discover_tests(test_vul_dispatch_table)
