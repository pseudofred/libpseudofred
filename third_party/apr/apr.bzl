load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _apr_deps():
    http_archive(
        name = "apr",
        url = "https://dlcdn.apache.org//apr/apr-1.7.6.tar.gz",
        sha256 = "6a10e7f7430510600af25fabf466e1df61aaae910bf1dc5d10c44a4433ccc81d",
        strip_prefix = "apr-1.7.6",
        build_file = "//third_party/apr:apr.BUILD",
    )

def _apr_deps_impl(_ctx):
    _apr_deps()

apr_deps = module_extension(
    implementation = _apr_deps_impl,
)
