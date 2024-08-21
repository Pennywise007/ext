load("@rules_cc//cc:defs.bzl", "cc_test")

cc_library(
   name = "serialization_samples",
   hdrs = ["samples_helper.h"],
   includes = ["."],
   data = glob([
      "serialization/*.txt",
   ]),
   visibility = ["//tests:__subpackages__"],
)
