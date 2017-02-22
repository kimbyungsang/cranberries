load(":integration_test.bzl", "integration_test")

integration_test("model_loading_test")

py_library(
  name = "utils",
  testonly = 1,
  srcs = ["utils.py"],
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
