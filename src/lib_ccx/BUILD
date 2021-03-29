cc_library(
    name = "lib_ccx",
    # zvbi has been modified a lot (it has a decoder context) so we can't
    # really treat as a 3rd party library
    srcs = glob (["*.c", "zvbi/*.c"]),
    hdrs = glob (["*.h", "zvbi/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
            "//src/thirdparty/protobuf-c:protobuf-c",
            "//src/thirdparty/libpng:libpng",
    ],
    includes = [ "../thirdparty/protobuf-c", "../thirdparty/libpng"  ]
)


