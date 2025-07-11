load("@rules_foreign_cc//foreign_cc:configure.bzl", "configure_make")

filegroup(
    name = "srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "apr",
    lib_source = ":srcs",
    copts = ["-lm"],
    out_static_libs = ["libapr-1.a"],
    #    targets = ["install"],
    visibility = ["//visibility:public"],
)
