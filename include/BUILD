load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "ext",
    hdrs = glob([
        "ext/**/*.h",
        "ext/*.h",
    ]),
    includes = ["."],
    linkopts = [
        "-luuid", # for uuid_t
    ],
    visibility = ["//visibility:public"],
)
