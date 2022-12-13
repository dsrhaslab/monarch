load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

pybind_extension(
    name = "_py_pastor", 
    srcs = ["module_binding.cpp"],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//:pastor"
    ],
)

py_library(
    name = "py_pastor",
    data = [":_py_pastor.so"],
)
