COMPILER_OPTIONS = [
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Werror",
    "-fmax-errors=3"
]

# Common ext tests definition with extra checks enabled
def ext_test(copts = [], **kwargs):
    native.cc_test(
        copts = COMPILER_OPTIONS + copts,
        deps = ["//tests:test_main"] + kwargs.pop("deps", []),
        **kwargs
    )