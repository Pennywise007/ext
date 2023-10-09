load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
   name = "serialization_test",
   srcs = ["serialization_test.cpp"],
   deps = [
      "//tests/samples:serialization_samples",
      "//tests:test_main",
      "@com_github_zeux_pugixml//:lib_pugixml",
   ],
)
