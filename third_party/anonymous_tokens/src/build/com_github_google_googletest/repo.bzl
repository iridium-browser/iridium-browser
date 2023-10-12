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
Repository rules/macros for com_github_google_googletest.
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def com_github_google_googletest_repo():
    if "com_github_google_googletest" not in native.existing_rules():
        http_archive(
            name = "com_github_google_googletest",
            sha256 = "dbca7a1d9bc8ac3a07dc6f667cbf540e468ec79f5cfebdadb8a57925078a450a",
            strip_prefix = "googletest-bc860af08783b8113005ca7697da5f5d49a8056f",
            url = "https://github.com/google/googletest/archive/bc860af08783b8113005ca7697da5f5d49a8056f.zip",
        )
