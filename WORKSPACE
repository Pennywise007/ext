load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "googletest",
    urls = ["https://github.com/google/googletest/archive/refs/tags/release-1.12.0.zip"],
    strip_prefix = "googletest-release-1.12.0",
)

http_archive(
    name = "com_github_zeux_pugixml",
    url = "https://github.com/zeux/pugixml/archive/v1.13.tar.gz",
    strip_prefix = "pugixml-1.13",
    build_file = "//bazel:BUILD.pugixml",
)
