load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "ext",
    hdrs = glob([
        "ext/**/*.h",
        "ext/*.h",
    ]),
    includes = ["."],
    linkopts = select({
        "@platforms//os:windows": [],
        # for uuid_t
        "//conditions:default": [ "-luuid", ],
    }),
    visibility = ["//visibility:public"],
)
