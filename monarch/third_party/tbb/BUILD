package(
    default_visibility = ["//visibility:public"],
    licenses = ["notice"],  # Apache 2.0
)

exports_files(["LICENSE.txt"])

cc_library(
    name = "tbb",
        hdrs = glob([
            "include/**/*.h",
        ]),
        srcs = ["lib/libtbb.so",
		"lib/libtbb.so.2"
	]
)

