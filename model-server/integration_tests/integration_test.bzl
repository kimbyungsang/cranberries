def integration_test(name):
  native.sh_test(
    name = "model_loading_test",
    srcs = ["run_py_test.sh"],
    data = [":" + name + "_impl"],
    deps = [":run_all_servers"],
  )

  native.py_library(
    name = name + "_impl",
    testonly = 1,
    srcs = [name + "_impl.py"],
    deps = [":utils"],
  )
