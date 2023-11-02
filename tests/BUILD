load("@rules_cc//cc:defs.bzl", "cc_library")

COMPILER_OPTIONS = [
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Werror",
    "-fmax-errors=3"
]

cc_library(
    name = "test_main",
    srcs = ["main.cpp"],
    deps = [
        "//include:ext",
        "@googletest//:gtest_main",
    ],
    copts = COMPILER_OPTIONS,
    visibility = ["//tests:__subpackages__"],
    features = ["-warning_flags"],
)
