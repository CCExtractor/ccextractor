#!/bin/bash
cd `dirname $0`

RUST_LIB="rust/release/libccx_rust.a"
RUST_PROFILE="--release"
RUST_FEATURES=""

while [[ $# -gt 0 ]]; do
  case $1 in
    OCR)
      ENABLE_OCR=true
      shift
      ;;
    -debug)
      DEBUG=true
      RUST_PROFILE=""
      RUST_LIB="rust/debug/libccx_rust.a"
      shift
      ;;
    -hardsubx)
      HARDSUBX=true
      RUST_FEATURES="--features hardsubx_ocr"
      shift
      ;;
    -*)
      echo "Unknown option $1"
      exit 1
      ;;
  esac
done

BLD_FLAGS="-std=gnu99 -Wno-write-strings -Wno-pointer-sign -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -Dfopen64=fopen -Dopen64=open -Dlseek64=lseek -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP"

if [[ "$DEBUG" == "true" ]]; then
    BLD_FLAGS="$BLD_FLAGS -g -fsanitize=address"
fi

if [[ "$ENABLE_OCR" == "true" ]]; then
    BLD_FLAGS="$BLD_FLAGS -DENABLE_OCR"
fi

if [[ "$HARDSUBX" == "true" ]]; then
    BLD_FLAGS="$BLD_FLAGS -DENABLE_HARDSUBX"
fi

BLD_INCLUDE="-I../src/ -I../src/lib_ccx -I../src/lib_hash -I../src/thirdparty/libpng -I../src/thirdparty -I../src/thirdparty/protobuf-c -I../src/thirdparty/zlib -I../src/thirdparty/freetype/include `pkg-config --cflags --silence-errors gpac`"

if [[ "$ENABLE_OCR" == "true" ]]; then
    BLD_INCLUDE="$BLD_INCLUDE `pkg-config --cflags --silence-errors tesseract`"
fi

SRC_CCX="$(find ../src/lib_ccx -name '*.c')"
SRC_LIB_HASH="$(find ../src/thirdparty/lib_hash -name '*.c')"
SRC_LIBPNG="$(find ../src/thirdparty/libpng -name '*.c')"
SRC_PROTOBUF="$(find ../src/thirdparty/protobuf-c -name '*.c')"
SRC_UTF8="../src/thirdparty/utf8proc/utf8proc.c"
SRC_ZLIB="$(find ../src/thirdparty/zlib -name '*.c')"
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

BLD_SOURCES="../src/ccextractor.c $SRC_API $SRC_CCX $SRC_LIB_HASH $SRC_LIBPNG $SRC_PROTOBUF $SRC_UTF8 $SRC_ZLIB $SRC_ZVBI $SRC_FREETYPE"

BLD_LINKER="-lm -liconv -lpthread -ldl `pkg-config --libs --silence-errors gpac`"

if [[ "$ENABLE_OCR" == "true" ]]; then
    BLD_LINKER="$BLD_LINKER `pkg-config --libs --silence-errors tesseract` `pkg-config --libs --silence-errors lept`"
fi

if [[ "$HARDSUBX" == "true" ]]; then
    BLD_LINKER="$BLD_LINKER -lswscale -lavutil -pthread -lavformat -lavcodec -lavfilter"
fi

echo "Running pre-build script..."
./pre-build.sh
echo "Trying to compile..."

echo "Checking for cargo..."
if ! [ -x "$(command -v cargo)" ]; then
    echo 'Error: cargo is not installed.' >&2
    exit 1
fi

rustc_version="$(rustc --version)"
semver=( ${rustc_version//./ } )
version="${semver[1]}.${semver[2]}.${semver[3]}"
MSRV="1.54.0"
if [ "$(printf '%s\n' "$MSRV" "$version" | sort -V | head -n1)" = "$MSRV" ]; then
    echo "rustc >= MSRV(${MSRV})"
else
    echo "Minimum supported rust version(MSRV) is ${MSRV}, please upgrade rust"
    exit 1
fi

echo "Building rust files..."
(cd ../src/rust && CARGO_TARGET_DIR=../../mac/rust cargo build $RUST_PROFILE $RUST_FEATURES) || { echo "Failed building Rust components." ; exit 1; }

cp $RUST_LIB ./libccx_rust.a

BLD_LINKER="$BLD_LINKER ./libccx_rust.a"

echo "Building ccextractor"
out=$( (LC_ALL=C gcc $BLD_FLAGS $BLD_INCLUDE -o ccextractor $BLD_SOURCES $BLD_LINKER) 2>&1)
res=$?

if [[ $out == *"gcc: command not found"* ]]; then
    echo "Error: please install gcc or Xcode command line tools"
    exit 1
fi

if [[ $out == *"curl.h: No such file or directory"* ]]; then
    echo "Error: please install curl development library"
    exit 2
fi

if [[ $out == *"capi.h: No such file or directory"* ]]; then
    echo "Error: please install tesseract development library"
    exit 3
fi

if [[ $out == *"allheaders.h: No such file or directory"* ]]; then
    echo "Error: please install leptonica development library"
    exit 4
fi

if [[ $res -ne 0 ]]; then
    echo "Compiled with errors"
    >&2 echo "$out"
    exit 5
fi

if [[ "$out" != "" ]]; then
    echo "$out"
    echo "Compilation successful, compiler message shown in previous lines"
else
    echo "Compilation successful, no compiler messages."
fi