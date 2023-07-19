package(default_visibility = ["//:__subpackages__"])

licenses(["notice"])

cc_library(
    name = "proto_utils",
    srcs = ["proto_utils.cc"],
    hdrs = ["proto_utils.h"],
    deps = [
        "//anonymous_tokens/proto:anonymous_tokens_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "status_utils",
    hdrs = ["status_utils.h"],
    deps = [
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status",
    ],
)
