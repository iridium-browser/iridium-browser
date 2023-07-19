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

project(securemessage)

option(securemessage_USE_LOCAL_PROTOBUF
       "Use local copy of protobuf library and compiler" OFF)

include(cmake/proto_defs.cmake)
include(cmake/local_build_setup.cmake)

if (securemessage_USE_LOCAL_PROTOBUF)
  include(cmake/local_build_protobuf.cmake)
endif()

find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

enable_testing()

add_subdirectory(cpp)
add_subdirectory(proto)
if (NOT TARGET gtest)
add_subdirectory(third_party/gtest)
endif()
