# Copyright 2022 The Centipede Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

workspace(name = "centipede")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

###############################################################################
# Bazel Skylib (transitively required by com_google_absl).
###############################################################################

skylib_ver = "1.2.1"

http_archive(
    name = "bazel_skylib",
    sha256 = "f7be3474d42aae265405a592bb7da8e171919d74c16f082a5457840f06054728",
    url = "https://github.com/bazelbuild/bazel-skylib/releases/download/%s/bazel-skylib-%s.tar.gz" % (skylib_ver, skylib_ver),
)

###############################################################################
# C++ build rules
# Configure the bootstrapped Clang and LLVM toolchain for Bazel.
###############################################################################

rules_cc_ver = "262ebec3c2296296526740db4aefce68c80de7fa"

http_archive(
    name = "rules_cc",
    sha256 = "9a446e9dd9c1bb180c86977a8dc1e9e659550ae732ae58bd2e8fd51e15b2c91d",
    strip_prefix = "rules_cc-%s" % rules_cc_ver,
    url = "https://github.com/bazelbuild/rules_cc/archive/%s.zip" % rules_cc_ver,
)

###############################################################################
# Abseil
###############################################################################

abseil_ref = ""

abseil_ver = "92fdbfb301f8b301b28ab5c99e7361e775c2fb8a"

# Use these values to get the tip of the master branch:
# abseil_ref = "refs/heads"
# abseil_ver = "master"

http_archive(
    name = "com_google_absl",
    sha256 = "71d38c5f44997a5ccbc338f904c8682b40c25cad60b9cbaf27087a917228d5fa",
    strip_prefix = "abseil-cpp-%s" % abseil_ver,
    url = "https://github.com/abseil/abseil-cpp/archive/%s/%s.tar.gz" % (abseil_ref, abseil_ver),
)

###############################################################################
# GoogleTest/GoogleMock
###############################################################################

# Version as of 2021-12-07.
googletest_ver = "4c5650f68866e3c2e60361d5c4c95c6f335fb64b"

http_archive(
    name = "com_google_googletest",
    sha256 = "770e61fa13d51320736c2881ff6279212e4eab8a9100709fff8c44759f61d126",
    strip_prefix = "googletest-%s" % googletest_ver,
    url = "https://github.com/google/googletest/archive/%s.tar.gz" % googletest_ver,
)
