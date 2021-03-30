cc_library(
    name = "lib_ccx",
    # zvbi has been modified a lot (it has a decoder context) so we can't
    # really treat as a 3rd party library
    srcs = glob (["*.c", "zvbi/*.c"]),
    hdrs = glob (["*.h", "zvbi/*.h", "*.xbm"]),
    visibility = ["//visibility:public"],
    deps = [
            "//src/thirdparty/protobuf-c:protobuf-c",
            "//src/thirdparty/libpng:libpng",
            "//src/thirdparty/freetype:freetype",
            "//src/thirdparty/gpacmp4:gpacmp4",
            "//src/thirdparty/lib_hash:lib_hash",
            "//src/thirdparty/utf8proc:utf8proc",
    ],
    includes = [ "thirdparty/protobuf-c", "thirdparty/libpng", "thirdparty/gpacmp4" , "." ],
    copts = [ "-Isrc/thirdparty/protobuf-c", 
              "-Isrc/thirdparty/libpng", 
              "-Isrc/", 
              "-Isrc/thirdparty/gpacmp4",
              "-Isrc/thirdparty/freetype",
              "-Isrc/thirdparty/lib_hash",

             ]
)


