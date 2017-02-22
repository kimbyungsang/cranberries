py_binary(
  name = "gen_models",
  testonly = 1,
  srcs = ["gen_models.py"],
  deps = [
    "@org_tensorflow//tensorflow:tensorflow_py",
  ],
)

py_binary(
  name = "client",
  srcs = ["client.py"],
  testonly = 1,
  deps = [":client_lib"],
)

py_library(
  name = "client_lib",
  testonly = 1,
  visibility = ["//visibility:public"],
  srcs = ["client_lib.py"],
  deps = [
    "@org_tensorflow//tensorflow:tensorflow_py",
    "@tf_serving//tensorflow_serving/apis:predict_proto_py_pb2",
    "@tf_serving//tensorflow_serving/apis:prediction_service_proto_py_pb2",
  ],
)

genrule(
  name = "test_models",
  testonly = 1,
  visibility = ["//visibility:public"],
  tools = [":gen_models"],
  outs = ["test_models.tar"],
  cmd = "./$(location :gen_models) $@",
)
