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

cmake_minimum_required(VERSION 3.0)

# libfuzzer needs clang
SET(CMAKE_C_COMPILER "clang")

add_compile_definitions("CORPUS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/corpus")
file(COPY corpus DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(generate_fuzz_corpus generate_fuzz_corpus.cc)
target_link_libraries(generate_fuzz_corpus nearby_protocol shared_test_util)

add_executable(fuzzer_np_cpp_deserialize fuzzer_np_cpp_deserialize.cc)
target_compile_options(fuzzer_np_cpp_deserialize PRIVATE -fsanitize=fuzzer,address)
target_link_libraries(fuzzer_np_cpp_deserialize nearby_protocol -fsanitize=fuzzer,address)

add_executable(fuzzer_np_cpp_valid_header fuzzer_np_cpp_valid_header.cc)
target_compile_options(fuzzer_np_cpp_valid_header PRIVATE -fsanitize=fuzzer,address)
target_link_libraries(fuzzer_np_cpp_valid_header nearby_protocol -fsanitize=fuzzer,address)
