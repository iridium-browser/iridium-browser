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

if(LIBGAV1_CMAKE_TOOLCHAINS_AARCH64_LINUX_GNU_CMAKE_)
  return()
endif() # LIBGAV1_CMAKE_TOOLCHAINS_AARCH64_LINUX_GNU_CMAKE_
set(LIBGAV1_CMAKE_TOOLCHAINS_AARCH64_LINUX_GNU_CMAKE_ 1)

set(CMAKE_SYSTEM_NAME "Linux")

if("${CROSS}" STREQUAL "")
  set(CROSS aarch64-linux-gnu-)
endif()

# For c_decoder_test.c and c_version_test.c.
if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER ${CROSS}gcc)
endif()
set(CMAKE_C_FLAGS_INIT "-march=armv8-a")
if(NOT CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER ${CROSS}g++)
endif()
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a")
set(CMAKE_SYSTEM_PROCESSOR "aarch64")
