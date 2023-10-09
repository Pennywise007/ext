load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "test_main",
    srcs = ["main.cpp"],
    deps = [
        "//include:ext",
        "@googletest//:gtest_main",
    ],
    visibility = ["//tests:__subpackages__"],
)
