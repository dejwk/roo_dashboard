load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "roo_dashboard",
    srcs = glob(
        [
            "src/**/*.cpp",
            "src/**/*.h",
        ],
        exclude = ["test/**"],
    ),
    includes = [
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@roo_windows",
    ],
)

cc_test(
    name = "radial_gauge_test",
    size = "small",
    srcs = ["test/radial_gauge_test.cpp"],
    deps = [":roo_dashboard", "@roo_testing//:arduino_gtest_main"],
)
