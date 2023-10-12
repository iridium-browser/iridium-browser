# Copyright 2023 Google LLC
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

# include ABSL third_party dir which will add it to the cmake build tree
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(${THIRD_PARTY_DIR}/abseil-cpp ${THIRD_PARTY_DIR}/abseil-cpp/build)

add_library(nearby_protocol nearby_protocol.cc)

target_link_libraries(nearby_protocol
        "${CMAKE_SOURCE_DIR}/np_c_ffi/target/release/${CMAKE_STATIC_LIBRARY_PREFIX}np_c_ffi${CMAKE_STATIC_LIBRARY_SUFFIX}"
        absl::statusor)

target_include_directories(nearby_protocol PUBLIC
        include/
        ${CMAKE_SOURCE_DIR}/np_c_ffi/include/cpp/)

target_include_directories(nearby_protocol PRIVATE
        ${CMAKE_SOURCE_DIR}/np_c_ffi/include/cpp/internal/)

add_subdirectory(sample)

# shared testing utilities between sample, tests, and fuzzers
add_library(shared_test_util shared/shared_test_util.cc)
target_include_directories(shared_test_util PUBLIC shared/)
target_include_directories(shared_test_util PRIVATE include/)
target_link_libraries(shared_test_util nearby_protocol absl::statusor)

if (ENABLE_TESTS)
    message(STATUS "Enabling np_cpp tests")
    add_subdirectory(tests)
    add_subdirectory(benchmarks)
endif ()

if (ENABLE_FUZZ)
    add_subdirectory(fuzz)
endif ()
