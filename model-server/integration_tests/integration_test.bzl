def integration_test(name, args=[]):
  native.sh_test(
    name = name,
    srcs = ["run_py_test.sh"],
    args = args,
    data = [":" + name + "_impl"],
    deps = [":run_all_servers"],
  )

  native.py_library(
    name = name + "_impl",
    testonly = 1,
    srcs = [name + "_impl.py"],
    deps = [":utils"],
  )
