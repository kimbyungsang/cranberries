cc_library(
  name = "zookeeper_cc",
  srcs = ["zookeeper_cc.cc"],
  hdrs = ["zookeeper_cc.h"],
  visibility = ["//visibility:public"],
  deps = [
    "@zookeeper//:zookeeper_mt",
  ],
)

sh_test(
  name = "zookeeper_cc_test",
  srcs = ["zookeeper_cc_test.sh"],
  size = "small",
  deps = [
    ":run_zookeeper_server",
  ],
  data = [
    ":zookeeper_cc_test_impl",
  ],
)

sh_library(
  name = "run_zookeeper_server",
  testonly = 1,
  srcs = ["run_zookeeper_server.sh"],
  deps = [
    "//sh_utils",
  ],
)

cc_binary(
  name = "zookeeper_cc_test_impl",
  srcs = ["zookeeper_cc_test.cc"],
  testonly = 1,
  deps = [
    ":zookeeper_cc",
    "//external:gtest_main",
  ],
)
