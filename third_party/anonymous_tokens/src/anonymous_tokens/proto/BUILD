# Public protos for Anonymous Tokens service.

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(default_visibility = ["//:__subpackages__"])

proto_library(
    name = "anonymous_tokens_proto",
    srcs = ["anonymous_tokens.proto"],
)

cc_proto_library(
    name = "anonymous_tokens_cc_proto",
    deps = [":anonymous_tokens_proto"],
)
