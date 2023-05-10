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

add_library(ukey2 STATIC
  d2d_connection_context_v1.cc
  d2d_crypto_ops.cc
  java_util.cc
  ukey2_handshake.cc
)

target_include_directories(ukey2
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src/main/cpp/include
)

target_link_libraries(ukey2
  PUBLIC
    proto_device_to_device_messages_cc_proto
    proto_securegcm_cc_proto
    proto_ukey_cc_proto
    securemessage
)

add_executable(ukey2_shell
  ukey2_shell.cc
)

target_link_libraries(ukey2_shell
  PUBLIC
    securemessage
    ukey2
    absl::base
    absl::container
    absl::flags
    absl::flags_parse
)
