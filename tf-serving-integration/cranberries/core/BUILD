cc_library(
  name = "zookeeper_source",
  srcs = ["zookeeper_source.cc"],
  hdrs = ["zookeeper_source.h"],
  visibility = ["//visibility:public"],
  deps = [
    "//zookeeper_cc_util",
    "@org_tensorflow//tensorflow/core:lib",
    "@tf_serving//tensorflow_serving/core:source",
    "@tf_serving//tensorflow_serving/core:storage_path",
  ],
)

cc_library(
  name = "zookeeper_state_reporter",
  srcs = ["zookeeper_state_reporter.cc"],
  hdrs = ["zookeeper_state_reporter.h"],
  visibility = ["//visibility:public"],
  deps = [
    "//zookeeper_cc_util",
    "@org_tensorflow//tensorflow/core:lib",
    "@tf_serving//tensorflow_serving/util:event_bus",
    "@tf_serving//tensorflow_serving/core:servable_state",
  ],
)
