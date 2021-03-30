cc_binary(
    name = "ccextractor",
    srcs = ["ccextractor.c",
            "ccextractor.h"],
    deps = [
            "//src/lib_ccx:lib_ccx",
            "//src/thirdparty/protobuf-c:protobuf-c"
    ],
    copts = [ "-Isrc/thirdparty/protobuf-c", "-Isrc/thirdparty/libpng", "-Isrc" ]
)

export_files =(["ccextractor.h", ["//src/lib_ccx:__pkg__"])

