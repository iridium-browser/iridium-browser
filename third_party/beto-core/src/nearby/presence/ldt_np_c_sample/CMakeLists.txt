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

include_directories(
    ${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/include/
    ${CMAKE_SOURCE_DIR}/ldt_np_c_sample/)

include(ExternalProject)
set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/target/tmp)
ExternalProject_Add(
    ldt_np_adv_ffi
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build COMMAND cargo build --release --no-default-features --features std
    BINARY_DIR "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi"
    INSTALL_COMMAND "")

add_executable(ldt_c_sample main.c)
target_link_libraries(
    ldt_c_sample
    optimized "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/target/release/${CMAKE_STATIC_LIBRARY_PREFIX}ldt_np_adv_ffi${CMAKE_STATIC_LIBRARY_SUFFIX}"
    debug "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/target/debug/${CMAKE_STATIC_LIBRARY_PREFIX}ldt_np_adv_ffi${CMAKE_STATIC_LIBRARY_SUFFIX}"
    OpenSSL::SSL
)

if(UNIX)
  target_link_libraries(
    ldt_c_sample
    dl pthread
  )
elseif(MSVC)
  target_link_libraries(
    ldt_c_sample
    bcrypt ntdll userenv ws2_32
  )
endif()


if(ENABLE_TESTS)
    message(STATUS "Enabling ldt tests")
    add_subdirectory(tests)
endif()
