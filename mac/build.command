#!/bin/bash
cd `dirname $0`
BLD_FLAGS="-std=gnu99 -Wno-write-strings -DGPAC_CONFIG_DARWIN -D_FILE_OFFSET_BITS=64 -Dfopen64=fopen -Dopen64=open -Dlseek64=lseek"
BLD_INCLUDE="-I../src/lib_ccx -I../src/gpacmp4 -I../src/libpng -I../src/zlib -I../src/zvbi -I../src/lib_hash"
SRC_LIBPNG="$(find ../src/libpng -name '*.c')"
SRC_ZLIB="$(find ../src/zlib -name '*.c')"
SRC_ZVBI="$(find ../src/zvbi -name '*.c')"
SRC_CCX="$(find ../src/lib_ccx -name '*.c')"
SRC_GPAC="$(find ../src/gpacmp4 -name '*.c')"
SRC_LIB_HASH="$(find ../src/lib_hash -name '*.c')"
BLD_SOURCES="../src/ccextractor.c $SRC_CCX $SRC_GPAC $SRC_ZVBI $SRC_ZLIB $SRC_LIBPNG $SRC_LIB_HASH"
BLD_LINKER="-lm -liconv"

gcc $BLD_FLAGS $BLD_INCLUDE -o ccextractor $BLD_SOURCES $BLD_LINKER


