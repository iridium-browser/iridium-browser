package(default_visibility = ["//:__subpackages__"])

licenses(["notice"])

cc_library(
    name = "blinder",
    hdrs = ["blinder.h"],
    deps = [
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_library(
    name = "constants",
    hdrs = [
        "constants.h",
    ],
    deps = [
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "crypto_utils",
    srcs = [
        "crypto_utils.cc",
    ],
    hdrs = [
        "crypto_utils.h",
    ],
    deps = [
        ":constants",
        "//anonymous_tokens/cpp/shared:status_utils",
        "//anonymous_tokens/proto:anonymous_tokens_cc_proto",
        "@boringssl//:ssl",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "rsa_blinder",
    srcs = ["rsa_blinder.cc"],
    hdrs = ["rsa_blinder.h"],
    deps = [
        ":blinder",
        ":constants",
        ":crypto_utils",
        "//anonymous_tokens/cpp/shared:status_utils",
        "//anonymous_tokens/proto:anonymous_tokens_cc_proto",
        "@boringssl//:ssl",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "blind_signer",
    hdrs = ["blind_signer.h"],
    deps = [
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "rsa_blind_signer",
    srcs = ["rsa_blind_signer.cc"],
    hdrs = ["rsa_blind_signer.h"],
    deps = [
        ":blind_signer",
        ":constants",
        ":crypto_utils",
        "//anonymous_tokens/cpp/shared:status_utils",
        "//anonymous_tokens/proto:anonymous_tokens_cc_proto",
        "@boringssl//:ssl",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "verifier",
    hdrs = ["verifier.h"],
    deps = [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "rsa_ssa_pss_verifier",
    srcs = ["rsa_ssa_pss_verifier.cc"],
    hdrs = ["rsa_ssa_pss_verifier.h"],
    deps = [
        ":constants",
        ":crypto_utils",
        ":verifier",
        "//anonymous_tokens/cpp/shared:status_utils",
        "//anonymous_tokens/proto:anonymous_tokens_cc_proto",
        "@boringssl//:ssl",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)
