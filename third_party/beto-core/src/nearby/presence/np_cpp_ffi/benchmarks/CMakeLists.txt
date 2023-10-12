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

add_executable(np_ffi_bench np_ffi_bench.cc)

target_link_libraries(
        np_ffi_bench
        nearby_protocol
        benchmark::benchmark)

if (UNIX)
    target_link_libraries(
            np_ffi_bench
            dl -fsanitize=address
    )
elseif (MSVC)
    target_link_libraries(
            np_ffi_bench
            bcrypt ntdll userenv ws2_32
    )
endif ()
