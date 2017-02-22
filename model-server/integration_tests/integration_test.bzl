def integration_test(name, args=[]):
  native.sh_test(
    name = name,
    srcs = ["run_py_test.sh"],
    args = args,
    data = [
      ":" + name + "_impl",
      "//integration_tests/test_models:test_models",
    ],
    deps = [
      ":run_model_server",
      "//zookeeper_cc:run_zookeeper_server",
    ],
  )

  native.py_library(
    name = name + "_impl",
    testonly = 1,
    srcs = [name + "_impl.py"],
    deps = [":utils"],
  )
