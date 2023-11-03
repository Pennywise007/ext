load("@rules_cc//cc:defs.bzl", "cc_library")
load(":extensions.bzl", "DEFAULT_COMPILER_OPTIONS")

cc_library(
    name = "test_main",
    srcs = ["main.cpp"],
    copts = DEFAULT_COMPILER_OPTIONS,
    deps = [
        "//include:ext",
        "@googletest//:gtest_main",
    ],
    visibility = ["//tests:__subpackages__"],
    features = ["-warning_flags"],
)
