#!/bin/bash
cd `dirname $0`
BLD_FLAGS="-std=gnu99 -Wno-write-strings -DGPAC_CONFIG_DARWIN -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -Dfopen64=fopen -Dopen64=open -Dlseek64=lseek -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP"
[[ $1 = "OCR" ]] && BLD_FLAGS="$BLD_FLAGS -DENABLE_OCR"
BLD_INCLUDE="-I../src/ -I../src/lib_ccx  -I../src/gpacmp4 -I../src/lib_hash -I../src/libpng -I../src/utf8proc -I../src/protobuf-c -I../src/zlib -I../src/zvbi -I../src/freetype/include"
[[ $1 = "OCR" ]] && BLD_INCLUDE="$BLD_INCLUDE `pkg-config --cflags --silence-errors tesseract`"
SRC_CCX="$(find ../src/lib_ccx -name '*.c')"
SRC_GPAC="$(find ../src/gpacmp4 -name '*.c')"
SRC_LIB_HASH="$(find ../src/lib_hash -name '*.c')"
SRC_LIBPNG="$(find ../src/libpng -name '*.c')"
SRC_PROTOBUF="$(find ../src/protobuf-c -name '*.c')"
SRC_UTF8="../src/utf8proc/utf8proc.c"
SRC_ZLIB="$(find ../src/zlib -name '*.c')"
SRC_ZVBI="$(find ../src/zvbi -name '*.c')"
API_WRAPPERS="$(find ../src/wrappers/ -name '*.c')"
API_EXTRACTORS="$(find ../src/extractors/ -name '*.c')"
SRC_FREETYPE="../src/freetype/autofit/autofit.c \
		../src/freetype/base/ftbase.c \
		../src/freetype/base/ftbbox.c \
		../src/freetype/base/ftbdf.c \
		../src/freetype/base/ftbitmap.c \
		../src/freetype/base/ftcid.c \
		../src/freetype/base/ftfntfmt.c \
		../src/freetype/base/ftfstype.c \
		../src/freetype/base/ftgasp.c \
		../src/freetype/base/ftglyph.c \
		../src/freetype/base/ftgxval.c \
		../src/freetype/base/ftinit.c \
		../src/freetype/base/ftlcdfil.c \
		../src/freetype/base/ftmm.c \
		../src/freetype/base/ftotval.c \
		../src/freetype/base/ftpatent.c \
		../src/freetype/base/ftpfr.c \
		../src/freetype/base/ftstroke.c \
		../src/freetype/base/ftsynth.c \
		../src/freetype/base/ftsystem.c \
		../src/freetype/base/fttype1.c \
		../src/freetype/base/ftwinfnt.c \
		../src/freetype/bdf/bdf.c \
		../src/freetype/bzip2/ftbzip2.c \
		../src/freetype/cache/ftcache.c \
		../src/freetype/cff/cff.c \
		../src/freetype/cid/type1cid.c \
		../src/freetype/gzip/ftgzip.c \
		../src/freetype/lzw/ftlzw.c \
		../src/freetype/pcf/pcf.c \
		../src/freetype/pfr/pfr.c \
		../src/freetype/psaux/psaux.c \
		../src/freetype/pshinter/pshinter.c \
		../src/freetype/psnames/psnames.c \
		../src/freetype/raster/raster.c \
		../src/freetype/sfnt/sfnt.c \
		../src/freetype/smooth/smooth.c \
		../src/freetype/truetype/truetype.c \
		../src/freetype/type1/type1.c \
		../src/freetype/type42/type42.c \
		../src/freetype/winfonts/winfnt.c"
BLD_SOURCES="../src/ccextractor.c $SRC_API $SRC_CCX  $SRC_GPAC $SRC_LIB_HASH $SRC_LIBPNG $SRC_PROTOBUF $SRC_UTF8 $SRC_ZLIB $SRC_ZVBI $SRC_FREETYPE $API_WRAPPERS $API_EXTRACTORS" 
BLD_LINKER="-lm -liconv"
[[ $1 = "OCR" ]] && BLD_LINKER="$BLD_LINKER `pkg-config --libs --silence-errors tesseract` `pkg-config --libs --silence-errors lept`"

./pre-build.sh
gcc $BLD_FLAGS $BLD_INCLUDE -o ccextractor $BLD_SOURCES $BLD_LINKER
