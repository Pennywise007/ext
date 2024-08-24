DEFAULT_COMPILER_OPTIONS = select({
    "@platforms//os:windows": [
        "/std:c++20",
        "-W4",
        "-WX",
    ],
    "//conditions:default": [
        "-std=c++20",
        "-Wall",
        "-Wcast-align",
        "-Wconversion",
        "-Wextra",
        "-Wpedantic",
        "-Wshadow",
        "-Werror",
        "-Wunreachable-code",
        "-fmax-errors=3"
    ],
})

# Common ext tests definition with extra checks enabled
def ext_test(copts = [], **kwargs):
    native.cc_test(
        copts = DEFAULT_COMPILER_OPTIONS + copts,
        deps = ["//tests:test_main"] + kwargs.pop("deps", []),
        **kwargs
    )