cc_library(
  name = "zookeeper_cc",
  srcs = ["zookeeper_cc.cc"],
  hdrs = ["zookeeper_cc.h"],
  visibility = ["//visibility:public"],
  deps = [
    ":path_utils",
    "@zookeeper//:zookeeper_mt",
  ],
)

cc_library(
  name = "path_utils",
  srcs = ["path_utils.cc"],
  hdrs = ["path_utils.h"],
  visibility = ["//visibility:public"],
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
  visibility = ["//visibility:public"],
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

cc_test(
  name = "path_utils_test",
  srcs = ["path_utils_test.cc"],
  deps = [
    ":path_utils",
    "//external:gtest_main",
  ],
)
