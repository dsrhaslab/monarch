package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

proto_library(
    name = "controller_service_proto",
    srcs = ["protos/controller_service.proto"],
)

cc_proto_library(
    name = "controller_service_cc_proto",
    deps = [":controller_service_proto"],
)

cc_grpc_library(
    name = "controller_service_cc_grpc",
    srcs = [":controller_service_proto"],
    grpc_only = True,
    deps = [":controller_service_cc_proto"],
)


cc_library(
	name = "pastor",
	srcs = glob([
		"src/control_plane/**/*.cpp",
		"src/data_plane/interfaces/intrusive/**/*.cpp",
		"src/data_plane/logic/**/*.cpp",
		"src/helpers/**/*.cpp"
	]),
	hdrs = glob([
		"src/control_plane/**/*.h",
		"src/data_plane/interfaces/intrusive/**/*.h",
		"src/data_plane/logic/**/*.h",
		"src/helpers/**/*.h"
	]),
    defines = ["BAZEL_BUILD"],
	deps = [
		":controller_service_cc_grpc",
	    "//third_party/tbb:tbb",
		"//third_party/ctpl:ctpl",
        "//third_party/parallel_hashmap:parallel_hashmap",
		"//third_party/utils:utils",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
		"@com_github_grpc_grpc//:grpc++",
		"@com_google_absl//absl/container:flat_hash_map",
		"@com_google_absl//absl/base",
		"@com_google_absl//absl/strings"
	]
)
