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

load(":build_defs.bzl", "centipede_fuzz_target")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files([
    "centipede_main_test.sh",
    "multi_sanitizer_fuzz_target.cc",
])

################################################################################
#                        Fuzz targets for fuzz tests
################################################################################

centipede_fuzz_target(
    name = "abort_fuzz_target",
)

centipede_fuzz_target(
    name = "empty_fuzz_target",
)

# Simple fuzz target used for testing.
centipede_fuzz_target(
    name = "test_fuzz_target",
)

# Target instrumented with -fsanitize-coverage=trace-pc.
centipede_fuzz_target(
    name = "test_fuzz_target_trace_pc",
    srcs = ["test_fuzz_target.cc"],
    sancov = "trace-pc",
)

# Target built with non-pie.
centipede_fuzz_target(
    name = "test_fuzz_target_non_pie",
    srcs = ["test_fuzz_target.cc"],
    linkopts = ["-no-pie"],
)

# Target instrumented with -fsanitize-coverage=trace-pc.
centipede_fuzz_target(
    name = "abort_fuzz_target_trace_pc",
    srcs = ["abort_fuzz_target.cc"],
    sancov = "trace-pc",
)

# Test fuzz target with lots of threads.
centipede_fuzz_target(
    name = "threaded_fuzz_target",
    sancov = "trace-pc-guard",  # Only this instrumentation to ensure it works.
)

# A standalone binary with main() that is worth fuzzinng.
cc_binary(
    name = "standalone_fuzz_target_with_main",
    srcs = ["standalone_fuzz_target_with_main.cc"],
)

################################################################################
# This fuzz target is not currently used by any automated tests and is here for
# manual tests only.
################################################################################

centipede_fuzz_target(
    name = "multi_sanitizer_fuzz_target",
)

################################################################################
#                       Helper binaries for fuzz tests
################################################################################

sh_binary(
    name = "test_input_filter",
    srcs = ["test_input_filter.sh"],
)

################################################################################
#                                 Fuzz tests
################################################################################

cc_test(
    name = "coverage_test",
    srcs = ["coverage_test.cc"],
    data = [
        ":test_fuzz_target",
        ":test_fuzz_target_trace_pc",
        ":threaded_fuzz_target",
    ],
    deps = [
        "@centipede//:centipede_interface",
        "@centipede//:control_flow",
        "@centipede//:coverage",
        "@centipede//:defs",
        "@centipede//:environment",
        "@centipede//:execution_result",
        "@centipede//:feature",
        "@centipede//:logging",
        "@centipede//:test_util",
        "@centipede//:util",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "centipede_test",
    timeout = "long",
    srcs = ["centipede_test.cc"],
    data = [
        ":abort_fuzz_target",
        ":test_fuzz_target",
        ":test_input_filter",
    ],
    deps = [
        "@centipede//:blob_file",
        "@centipede//:centipede_callbacks",
        "@centipede//:centipede_interface",
        "@centipede//:corpus",
        "@centipede//:defs",
        "@centipede//:environment",
        "@centipede//:execution_result",
        "@centipede//:feature",
        "@centipede//:logging",
        "@centipede//:shard_reader",
        "@centipede//:test_util",
        "@centipede//:util",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest_main",
    ],
)

# Runs common Centipede scenarios with local files.
sh_test(
    name = "centipede_main_test",
    timeout = "long",
    srcs = ["centipede_main_test.sh"],
    data = [
        ":abort_fuzz_target",
        ":test_fuzz_target",
        "@centipede",
        "@centipede//:test_util_sh",
    ],
)

sh_test(
    name = "runner_test",
    srcs = ["runner_test.sh"],
    data = [
        ":test_fuzz_target",
        ":test_fuzz_target_non_pie",
        "@centipede//:test_util_sh",
    ],
)

sh_test(
    name = "instrumentation_test",
    srcs = ["instrumentation_test.sh"],
    data = [
        ":empty_fuzz_target",
        "@centipede//:test_util_sh",
    ],
)

sh_test(
    name = "trace_pc_test",
    srcs = ["trace_pc_test.sh"],
    data = [
        ":abort_fuzz_target_trace_pc",
        "@centipede",
        "@centipede//:test_util_sh",
    ],
)
