# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Repository rules/macros for tink_cc.
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def tink_cc_repo():
    if "tink_cc" not in native.existing_rules():
        http_archive(
            name = "tink_cc",
            sha256 = "3994412ebe46526813153f0490295d720c7949af9687e2d2e5480097aff3612a",
            strip_prefix = "tink-1.6.1/cc",
            url = "https://github.com/google/tink/archive/v1.6.1.zip",
        )
