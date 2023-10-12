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

file(COPY ${CMAKE_SOURCE_DIR}/ldt_np_adv/resources/test/np_adv_test_vectors.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(
  ldt_ffi_tests
  ldt_ffi_tests.cc
)

target_link_libraries(
  ldt_ffi_tests
  "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/target/release/${CMAKE_STATIC_LIBRARY_PREFIX}ldt_np_adv_ffi${CMAKE_STATIC_LIBRARY_SUFFIX}"
  jsoncpp
  GTest::gtest_main
  OpenSSL::SSL
)
if(UNIX)
  target_link_libraries(
    ldt_ffi_tests
    dl pthread -fsanitize=address
  )
elseif(MSVC)
  target_link_libraries(
    ldt_ffi_tests
    bcrypt ntdll userenv ws2_32
  )
endif()

gtest_discover_tests(ldt_ffi_tests)

# Benchmarks
add_executable(
  ldt_benchmarks
  ldt_benchmarks.cc
)

target_link_libraries(
  ldt_benchmarks
  "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/target/release/${CMAKE_STATIC_LIBRARY_PREFIX}ldt_np_adv_ffi${CMAKE_STATIC_LIBRARY_SUFFIX}"
  benchmark::benchmark
  OpenSSL::SSL
)

if(UNIX)
  target_link_libraries(
    ldt_benchmarks
    dl pthread -fsanitize=address
  )
elseif(MSVC)
  target_link_libraries(
    ldt_benchmarks
    bcrypt ntdll userenv ws2_32
  )
endif()
