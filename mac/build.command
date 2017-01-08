#!/bin/bash
cd `dirname $0`
BLD_FLAGS="-std=gnu99 -Wno-write-strings -DGPAC_CONFIG_DARWIN -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -Dfopen64=fopen -Dopen64=open -Dlseek64=lseek"
BLD_INCLUDE="-I../src/ -I../src/lib_ccx  -I../src/gpacmp4 -I../src/lib_hash -I../src/libpng -I../src/utf8proc -I../src/protobuf-c -I../src/zlib -I../src/zvbi -I /usr/local/include/leptonica/ -I /usr/local/include/tesseract/"
SRC_CCX="$(find ../src/lib_ccx -name '*.c')"
SRC_GPAC="$(find ../src/gpacmp4 -name '*.c')"
SRC_LIB_HASH="$(find ../src/lib_hash -name '*.c')"
SRC_LIBPNG="$(find ../src/libpng -name '*.c')"
SRC_PROTOBUF="$(find ../src/protobuf-c -name '*.c')"
SRC_UTF8="$(find ../src/utf8proc-c -name '*.c')"
SRC_ZLIB="$(find ../src/zlib -name '*.c')"
SRC_ZVBI="$(find ../src/zvbi -name '*.c')"
BLD_SOURCES="../src/ccextractor.c $SRC_CCX  $SRC_GPAC $SRC_LIB_HASH $SRC_LIBPNG $SRC_PROTOBUF $SRC_UTF8 $SRC_ZLIB $SRC_ZVBI" 
BLD_LINKER="-lm -liconv"

./pre-build.sh
gcc $BLD_FLAGS $BLD_INCLUDE -o ccextractor $BLD_SOURCES $BLD_LINKER
