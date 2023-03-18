cc_library(
    name = "gpacmp4",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h", "gpac/*.h", "gpac/internal/*.h", "gpac/modules/*.h" ]),
    copts = [ "-I src/thirdparty/gpacmp4/" , "-DGPAC_CONFIG_LINUX -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -DENABLE_OCR -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP -DGPAC_HAVE_CONFIG_H -DGPAC_DISABLE_ISOM_DUMP -DGPAC_DISABLE_MEDIA_EXPORT" ],
    visibility = ["//visibility:public"],
    linkopts = ["-ldl", "-lpthread"],
)

