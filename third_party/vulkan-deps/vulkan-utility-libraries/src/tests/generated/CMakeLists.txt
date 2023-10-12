# Copyright 2023 The Khronos Group Inc.
# Copyright 2023 Valve Corporation
# Copyright 2023 LunarG, Inc.
#
# SPDX-License-Identifier: Apache-2.0
set(CMAKE_FOLDER "${CMAKE_FOLDER}/generated_code/tests")

find_package(magic_enum REQUIRED CONFIG)
find_package(GTest REQUIRED CONFIG)

include(GoogleTest)

if(${CMAKE_C_COMPILER_ID} MATCHES "(GNU|Clang)")
    add_compile_options(-Wpedantic -Wall -Wextra -Werror)
endif()

if (MSVC)
    add_compile_options(/W4 /permissive- /WX)
endif()

# Test vk_enum_string_helper.h
add_executable(vk_enum_string_helper vk_enum_string_helper.cpp)
target_include_directories(vk_enum_string_helper PRIVATE ${VUL_SOURCE_DIR}/include)
target_link_libraries(vk_enum_string_helper PRIVATE
    Vulkan::Headers
    GTest::gtest
    GTest::gtest_main
    magic_enum::magic_enum
)

gtest_discover_tests(vk_enum_string_helper)

