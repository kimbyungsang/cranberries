load("@protobuf//:protobuf.bzl", "cc_proto_library")

SUPPORTED_TENSORFLOW_OPS = [
  "@org_tensorflow//tensorflow/contrib:contrib_kernels",
  "@org_tensorflow//tensorflow/contrib:contrib_ops_op_lib",
]

TENSORFLOW_DEPS = [
  "@org_tensorflow//tensorflow/core:tensorflow",
  "@tf_serving//tensorflow_serving/servables/tensorflow:saved_model_bundle_source_adapter",
  "@tf_serving//tensorflow_serving/servables/tensorflow:session_bundle_source_adapter",
]

cc_binary(
  name = "model_server",
  srcs = [
    "model_server.cc",
  ],
  visibility = ["//visibility:public"],
  deps = [
    "@tf_serving//tensorflow_serving/model_servers:model_platform_types",
    "@tf_serving//tensorflow_serving/model_servers:platform_config_util",
    "@tf_serving//tensorflow_serving/model_servers:server_core",
    "@tf_serving//tensorflow_serving/apis:prediction_service_proto",
    "@tf_serving//tensorflow_serving/config:model_server_config_proto",
    "@tf_serving//tensorflow_serving/core:availability_preserving_policy",
    "@org_tensorflow//tensorflow/core:lib",
    "@org_tensorflow//tensorflow/core/platform/cloud:gcs_file_system",
    "@protobuf//:cc_wkt_protos",
    "@grpc//:grpc++",
    ":model_server_config_cc_lib",
    "//cranberries/core:zookeeper_source",
    "//cranberries/core:zookeeper_state_reporter",
    "//zookeeper_cc",
    ":predict_impl",
  ] + TENSORFLOW_DEPS + SUPPORTED_TENSORFLOW_OPS,
)

cc_library(
    name = "predict_impl",
    srcs = ["predict_impl.cc"],
    hdrs = ["predict_impl.h"],
    deps = [
        "@tf_serving//tensorflow_serving/servables/tensorflow:get_model_metadata_impl",
        "@tf_serving//tensorflow_serving/apis:get_model_metadata_proto",
        "@tf_serving//tensorflow_serving/apis:predict_proto",
        "@tf_serving//tensorflow_serving/core:servable_handle",
        "@tf_serving//tensorflow_serving/model_servers:server_core",
        "@org_tensorflow//tensorflow/cc/saved_model:loader",
        "@org_tensorflow//tensorflow/cc/saved_model:signature_constants",
        "@org_tensorflow//tensorflow/contrib/session_bundle",
        "@org_tensorflow//tensorflow/contrib/session_bundle:signature",
        "@org_tensorflow//tensorflow/core:lib",
        "@org_tensorflow//tensorflow/core:protos_all_cc",
    ],
)

cc_proto_library(
  name = "model_server_config_cc_lib",
  srcs = ["model_server_config.proto"],
  cc_libs = ["@protobuf//:protobuf"],
  protoc = "@protobuf//:protoc",
  default_runtime = "@protobuf//:protobuf",
)
