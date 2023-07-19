# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.0.2)

project(ukey2)

option(ukey2_USE_LOCAL_PROTOBUF
       "Use local copy of protobuf library and compiler" OFF)

option(ukey2_USE_LOCAL_ABSL
       "Use local copy of abseil-cpp library" OFF)

include(cmake/proto_defs.cmake)
include(cmake/local_build_setup.cmake)

if (ukey2_USE_LOCAL_PROTOBUF)
  include(cmake/local_build_protobuf.cmake)
endif()

if (ukey2_USE_LOCAL_ABSL)
  if (NOT TARGET absl::base)
    add_subdirectory(third_party/absl)
  endif()
else()
  find_package(absl REQUIRED)
endif()

find_package(Protobuf REQUIRED)

enable_testing()

add_subdirectory(src/main)
if (NOT TARGET securemessage)
add_subdirectory(third_party/secure_message)
endif()
if (NOT TARGET gtest)
add_subdirectory(third_party/gtest)
endif()
