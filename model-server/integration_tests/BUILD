sh_test(
  name = "model_loading_test",
  srcs = ["run_py_test.sh"],
  data = [":model_loading_test_impl"],
  deps = [":run_all_servers"],
)

py_binary(
  name = "model_loading_test_impl",
  testonly = 1,
  srcs = ["model_loading_test_impl.py"],
)

sh_library(
  name = "run_all_servers",
  srcs = ["run_all_servers.sh"],
  testonly = 1,
  data = [
    "//integration_tests/test_models:test_models",
  ],
  deps = [
    ":run_model_server",
    "//zookeeper_cc:run_zookeeper_server",
  ]
)

sh_library(
  name = "run_model_server",
  srcs = ["run_model_server.sh"],
  testonly = 1,
  data = [
    "//cranberries/model_server",
  ],
  deps = [
    "//sh_utils",
  ],
)
