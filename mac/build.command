#!/bin/bash
cd `dirname $0`
BLD_FLAGS="-std=gnu99 -Wno-write-strings -Wno-pointer-sign -DGPAC_CONFIG_DARWIN -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -Dfopen64=fopen -Dopen64=open -Dlseek64=lseek -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP -DGPAC_HAVE_CONFIG_H"
[[ $1 = "OCR" ]] && BLD_FLAGS="$BLD_FLAGS -DENABLE_OCR"
BLD_INCLUDE="-I../src/ -I../src/lib_ccx  -I../src/thirdparty/gpacmp4 -I../src/lib_hash -I../src/thirdparty/libpng -I../src/thirdparty -I../src/thirdparty/protobuf-c -I../src/thirdparty/zlib -I../src/thirdparty/zvbi -I../src/thirdparty/freetype/include"
[[ $1 = "OCR" ]] && BLD_INCLUDE="$BLD_INCLUDE `pkg-config --cflags --silence-errors tesseract`"
SRC_CCX="$(find ../src/lib_ccx -name '*.c')"
SRC_GPAC="$(find ../src/thirdparty/gpacmp4 -name '*.c')"
SRC_LIB_HASH="$(find ../src/thirdparty/lib_hash -name '*.c')"
SRC_LIBPNG="$(find ../src/thirdparty/libpng -name '*.c')"
SRC_PROTOBUF="$(find ../src/thirdparty/protobuf-c -name '*.c')"
SRC_UTF8="../src/thirdparty/utf8proc/utf8proc.c"
SRC_ZLIB="$(find ../src/thirdparty/zlib -name '*.c')"
SRC_ZVBI="$(find ../src/thirdparty/zvbi -name '*.c')"
SRC_FREETYPE="../src/thirdparty/freetype/autofit/autofit.c \
		../src/thirdparty/freetype/base/ftbase.c \
		../src/thirdparty/freetype/base/ftbbox.c \
		../src/thirdparty/freetype/base/ftbdf.c \
		../src/thirdparty/freetype/base/ftbitmap.c \
		../src/thirdparty/freetype/base/ftcid.c \
		../src/thirdparty/freetype/base/ftfntfmt.c \
		../src/thirdparty/freetype/base/ftfstype.c \
		../src/thirdparty/freetype/base/ftgasp.c \
		../src/thirdparty/freetype/base/ftglyph.c \
		../src/thirdparty/freetype/base/ftgxval.c \
		../src/thirdparty/freetype/base/ftinit.c \
		../src/thirdparty/freetype/base/ftlcdfil.c \
		../src/thirdparty/freetype/base/ftmm.c \
		../src/thirdparty/freetype/base/ftotval.c \
		../src/thirdparty/freetype/base/ftpatent.c \
		../src/thirdparty/freetype/base/ftpfr.c \
		../src/thirdparty/freetype/base/ftstroke.c \
		../src/thirdparty/freetype/base/ftsynth.c \
		../src/thirdparty/freetype/base/ftsystem.c \
		../src/thirdparty/freetype/base/fttype1.c \
		../src/thirdparty/freetype/base/ftwinfnt.c \
		../src/thirdparty/freetype/bdf/bdf.c \
		../src/thirdparty/freetype/bzip2/ftbzip2.c \
		../src/thirdparty/freetype/cache/ftcache.c \
		../src/thirdparty/freetype/cff/cff.c \
		../src/thirdparty/freetype/cid/type1cid.c \
		../src/thirdparty/freetype/gzip/ftgzip.c \
		../src/thirdparty/freetype/lzw/ftlzw.c \
		../src/thirdparty/freetype/pcf/pcf.c \
		../src/thirdparty/freetype/pfr/pfr.c \
		../src/thirdparty/freetype/psaux/psaux.c \
		../src/thirdparty/freetype/pshinter/pshinter.c \
		../src/thirdparty/freetype/psnames/psnames.c \
		../src/thirdparty/freetype/raster/raster.c \
		../src/thirdparty/freetype/sfnt/sfnt.c \
		../src/thirdparty/freetype/smooth/smooth.c \
		../src/thirdparty/freetype/truetype/truetype.c \
		../src/thirdparty/freetype/type1/type1.c \
		../src/thirdparty/freetype/type42/type42.c \
		../src/thirdparty/freetype/winfonts/winfnt.c"
BLD_SOURCES="../src/ccextractor.c $SRC_API $SRC_CCX  $SRC_GPAC $SRC_LIB_HASH $SRC_LIBPNG $SRC_PROTOBUF $SRC_UTF8 $SRC_ZLIB $SRC_ZVBI $SRC_FREETYPE"
BLD_LINKER="-lm -liconv -lpthread -ldl"
[[ $1 = "OCR" ]] && BLD_LINKER="$BLD_LINKER `pkg-config --libs --silence-errors tesseract` `pkg-config --libs --silence-errors lept`"

./pre-build.sh
gcc $BLD_FLAGS $BLD_INCLUDE -o ccextractor $BLD_SOURCES $BLD_LINKER
