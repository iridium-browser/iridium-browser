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

cmake_minimum_required(VERSION 3.0)

# libfuzzer needs clang
SET (CMAKE_C_COMPILER "clang")

include_directories(
    ${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/include/)

# enable fuzzer with ASAN
# Can't use MSAN since EVP_KDF_fetch seems to read uninitialized memory, maybe on purpose?
add_compile_options(-fsanitize=fuzzer,address)

add_executable(fuzzer_decrypt_bad_mac src/fuzzer_decrypt_bad_mac.c)
target_link_libraries(fuzzer_decrypt_bad_mac
    optimized "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/target/release/libldt_np_adv_ffi.a"
    debug "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/target/debug/libldt_np_adv_ffi.a"
    dl pthread -fsanitize=fuzzer,address)
target_link_libraries(fuzzer_decrypt_bad_mac OpenSSL::SSL)

add_executable(fuzzer_decrypt_correct_mac src/fuzzer_decrypt_correct_mac.c)
target_link_libraries(fuzzer_decrypt_correct_mac
    optimized "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/target/release/libldt_np_adv_ffi.a"
    debug "${CMAKE_SOURCE_DIR}/ldt_np_adv_ffi/target/debug/libldt_np_adv_ffi.a"
    dl pthread -fsanitize=fuzzer,address)
target_link_libraries(fuzzer_decrypt_correct_mac OpenSSL::SSL)
