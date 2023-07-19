# Copyright 2018 Google LLC
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

licenses(["notice"])

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load(":data_blob.bzl", "cc_data_blob")

cc_data_blob(
    name = "font_data",
    src = "//third_party/fonts:DroidSans.ttf",
    varname = "kDroidSansBlob",
)

cc_library(
    name = "render_core",
    srcs = [
        "axis_renderer.cc",
        "program_state.cc",
        "rectangle_renderer.cc",
        "sdl_util.cc",
        "shader.cc",
        "table.cc",
        "text.cc",
        "trace_renderer.cc",
    ],
    hdrs = [
        "axis_renderer.h",
        "geometry_util.h",
        "layout_constants.h",
        "program_state.h",
        "rectangle_renderer.h",
        "sdl_util.h",
        "shader.h",
        "table.h",
        "text.h",
        "trace_renderer.h",
    ],
    deps = [
        ":font_data",
        "//third_party/glew",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@sdl2",
        "@sdl2_ttf",
    ],
)

cc_library(
    name = "render_program",
    srcs = [
        "processed_trace.cc",
        "trace_program.cc",
    ],
    hdrs = [
        "processed_trace.h",
        "trace_program.h",
    ],
    deps = [
        ":render_core",
        "//quic_trace:quic_trace_cc_proto",
        "//quic_trace/analysis:numbering",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "render_trace",
    srcs = [
        "render_trace.cc",
    ],
    deps = [
        ":render_program",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:flags",
        "@com_google_absl//absl/log:initialize",
        "@com_google_absl//absl/strings",
    ],
)

bzl_library(
    name = "data_blob_bzl",
    srcs = ["data_blob.bzl"],
    visibility = ["//visibility:private"],
)
