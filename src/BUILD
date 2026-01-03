cc_binary(
    name = "ccextractor",
    srcs = ["ccextractor.c",
            "ccextractor.h"],
    deps = [
            "//src/lib_ccx:lib_ccx",
            "//src/thirdparty/zlib:zlib",
            "//src/thirdparty/freetype:freetype"
    ],
    copts = [ "-Isrc/thirdparty/libpng", "-Isrc" ]
)

exports_files (["ccextractor.h"], ["//src/lib_ccx:__pkg__"])

