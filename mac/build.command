#!/bin/bash
cd `dirname $0`

RUST_LIB="rust/release/libccx_rust.a"
RUST_PROFILE="--release"
RUST_FEATURES=""

# Parse command line arguments
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
      ENABLE_OCR=true
      # Allow overriding FFmpeg version via environment variable
      if [ -n "$FFMPEG_VERSION" ]; then
        RUST_FEATURES="--features hardsubx_ocr,$FFMPEG_VERSION"
      else
        RUST_FEATURES="--features hardsubx_ocr"
      fi
      shift
      ;;
    -system-libs)
      # Use system-installed libraries via pkg-config instead of bundled ones
      # This is required for Homebrew formula compatibility
      USE_SYSTEM_LIBS=true
      shift
      ;;
    -*)
      echo "Unknown option $1"
      exit 1
      ;;
  esac
done

# Determine architecture based on cargo (to ensure consistency with Rust part)
CARGO_ARCH=$(file $(which cargo) | grep -o 'x86_64\|arm64')
if [[ "$CARGO_ARCH" == "x86_64" ]]; then
    echo "Detected Intel (x86_64) Cargo. Forcing x86_64 build to match Rust and libraries..."
    BLD_ARCH="-arch x86_64"
else
    BLD_ARCH="-arch arm64"
fi

BLD_FLAGS="$BLD_ARCH -std=gnu99 -Wno-write-strings -Wno-pointer-sign -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -Dfopen64=fopen -Dopen64=open -Dlseek64=lseek"

# Add flags for bundled libraries (not needed when using system libs)
if [[ "$USE_SYSTEM_LIBS" != "true" ]]; then
    BLD_FLAGS="$BLD_FLAGS -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP"
fi

# Add debug flags if needed
if [[ "$DEBUG" == "true" ]]; then
    BLD_FLAGS="$BLD_FLAGS -g -fsanitize=address"
fi

# Add OCR support if requested
if [[ "$ENABLE_OCR" == "true" ]]; then
    BLD_FLAGS="$BLD_FLAGS -DENABLE_OCR"
fi

# Add hardsubx support if requested
if [[ "$HARDSUBX" == "true" ]]; then
    BLD_FLAGS="$BLD_FLAGS -DENABLE_HARDSUBX"
fi

# Set up include paths based on whether we're using system libs or bundled
if [[ "$USE_SYSTEM_LIBS" == "true" ]]; then
    # Use system libraries via pkg-config (for Homebrew compatibility)
    # Note: -I../src/thirdparty/lib_hash is needed so that "../lib_hash/sha2.h" resolves correctly
    # (the .. goes up from lib_hash to thirdparty, then lib_hash/sha2.h finds the file)
    BLD_INCLUDE="-I../src/ -I../src/lib_ccx -I../src/thirdparty/lib_hash -I../src/thirdparty"
    BLD_INCLUDE="$BLD_INCLUDE $(pkg-config --cflags --silence-errors freetype2)"
    BLD_INCLUDE="$BLD_INCLUDE $(pkg-config --cflags --silence-errors gpac)"
    BLD_INCLUDE="$BLD_INCLUDE $(pkg-config --cflags --silence-errors libpng)"
    BLD_INCLUDE="$BLD_INCLUDE $(pkg-config --cflags --silence-errors libprotobuf-c)"
    BLD_INCLUDE="$BLD_INCLUDE $(pkg-config --cflags --silence-errors libutf8proc)"
else
    # Use bundled libraries (default for standalone builds)
    BLD_INCLUDE="-I../src/ -I../src/lib_ccx -I../src/thirdparty/lib_hash -I../src/thirdparty/libpng -I../src/thirdparty -I../src/thirdparty/zlib -I../src/thirdparty/freetype/include $(pkg-config --cflags --silence-errors gpac)"
fi

# Add FFmpeg include path for Mac
if [[ -d "/opt/homebrew/Cellar/ffmpeg" ]]; then
    FFMPEG_VERSION=$(ls -1 /opt/homebrew/Cellar/ffmpeg | head -1)
    if [[ -n "$FFMPEG_VERSION" ]]; then
        BLD_INCLUDE="$BLD_INCLUDE -I/opt/homebrew/Cellar/ffmpeg/$FFMPEG_VERSION/include"
    fi
elif [[ -d "/usr/local/Cellar/ffmpeg" ]]; then
    FFMPEG_VERSION=$(ls -1 /usr/local/Cellar/ffmpeg | head -1)
    if [[ -n "$FFMPEG_VERSION" ]]; then
        BLD_INCLUDE="$BLD_INCLUDE -I/usr/local/Cellar/ffmpeg/$FFMPEG_VERSION/include"
    fi
fi

# Add Leptonica include path for Mac
if [[ -d "/opt/homebrew/Cellar/leptonica" ]]; then
    LEPT_VERSION=$(ls -1 /opt/homebrew/Cellar/leptonica | head -1)
    if [[ -n "$LEPT_VERSION" ]]; then
        BLD_INCLUDE="$BLD_INCLUDE -I/opt/homebrew/Cellar/leptonica/$LEPT_VERSION/include"
    fi
elif [[ -d "/usr/local/Cellar/leptonica" ]]; then
    LEPT_VERSION=$(ls -1 /usr/local/Cellar/leptonica | head -1)
    if [[ -n "$LEPT_VERSION" ]]; then
        BLD_INCLUDE="$BLD_INCLUDE -I/usr/local/Cellar/leptonica/$LEPT_VERSION/include"
    fi
elif [[ -d "/opt/homebrew/include/leptonica" ]]; then
    BLD_INCLUDE="$BLD_INCLUDE -I/opt/homebrew/include"
elif [[ -d "/usr/local/include/leptonica" ]]; then
    BLD_INCLUDE="$BLD_INCLUDE -I/usr/local/include"
fi

# Add Tesseract include path for Mac  
if [[ -d "/opt/homebrew/Cellar/tesseract" ]]; then
    TESS_VERSION=$(ls -1 /opt/homebrew/Cellar/tesseract | head -1)
    if [[ -n "$TESS_VERSION" ]]; then
        BLD_INCLUDE="$BLD_INCLUDE -I/opt/homebrew/Cellar/tesseract/$TESS_VERSION/include"
    fi
elif [[ -d "/usr/local/Cellar/tesseract" ]]; then
    TESS_VERSION=$(ls -1 /usr/local/Cellar/tesseract | head -1)
    if [[ -n "$TESS_VERSION" ]]; then
        BLD_INCLUDE="$BLD_INCLUDE -I/usr/local/Cellar/tesseract/$TESS_VERSION/include"
    fi
elif [[ -d "/opt/homebrew/include/tesseract" ]]; then
    BLD_INCLUDE="$BLD_INCLUDE -I/opt/homebrew/include"
elif [[ -d "/usr/local/include/tesseract" ]]; then
    BLD_INCLUDE="$BLD_INCLUDE -I/usr/local/include"
fi

if [[ "$ENABLE_OCR" == "true" ]]; then
    BLD_INCLUDE="$BLD_INCLUDE `pkg-config --cflags --silence-errors tesseract`"
fi

SRC_CCX="$(find ../src/lib_ccx -name '*.c')"
SRC_LIB_HASH="$(find ../src/thirdparty/lib_hash -name '*.c')"

# Set up sources and linker based on whether we're using system libs or bundled
if [[ "$USE_SYSTEM_LIBS" == "true" ]]; then
    # Use system libraries - don't compile bundled sources
    BLD_SOURCES="../src/ccextractor.c $SRC_CCX $SRC_LIB_HASH"

    BLD_LINKER="-lm -liconv -lpthread -ldl"
    BLD_LINKER="$BLD_LINKER $(pkg-config --libs --silence-errors freetype2)"
    BLD_LINKER="$BLD_LINKER $(pkg-config --libs --silence-errors gpac)"
    BLD_LINKER="$BLD_LINKER $(pkg-config --libs --silence-errors libpng)"
    BLD_LINKER="$BLD_LINKER $(pkg-config --libs --silence-errors libprotobuf-c)"
    BLD_LINKER="$BLD_LINKER $(pkg-config --libs --silence-errors libutf8proc)"
    BLD_LINKER="$BLD_LINKER $(pkg-config --libs --silence-errors zlib)"
else
    # Use bundled libraries (default)
    SRC_LIBPNG="$(find ../src/thirdparty/libpng -name '*.c')"
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

    BLD_SOURCES="../src/ccextractor.c $SRC_CCX $SRC_LIB_HASH $SRC_LIBPNG $SRC_UTF8 $SRC_ZLIB $SRC_FREETYPE"
    BLD_LINKER="-lm -liconv -lpthread -ldl $(pkg-config --libs --silence-errors gpac)"
fi

if [[ "$ENABLE_OCR" == "true" ]]; then
    BLD_LINKER="$BLD_LINKER `pkg-config --libs --silence-errors tesseract` `pkg-config --libs --silence-errors lept`"
fi

if [[ "$HARDSUBX" == "true" ]]; then
    # Add FFmpeg library path for Mac
    if [[ -d "/opt/homebrew/Cellar/ffmpeg" ]]; then
        FFMPEG_VERSION=$(ls -1 /opt/homebrew/Cellar/ffmpeg | head -1)
        if [[ -n "$FFMPEG_VERSION" ]]; then
            BLD_LINKER="$BLD_LINKER -L/opt/homebrew/Cellar/ffmpeg/$FFMPEG_VERSION/lib"
        fi
    elif [[ -d "/usr/local/Cellar/ffmpeg" ]]; then
        FFMPEG_VERSION=$(ls -1 /usr/local/Cellar/ffmpeg | head -1)
        if [[ -n "$FFMPEG_VERSION" ]]; then
            BLD_LINKER="$BLD_LINKER -L/usr/local/Cellar/ffmpeg/$FFMPEG_VERSION/lib"
        fi
    fi
    
    # Add library paths for Leptonica and Tesseract from Cellar
    if [[ -d "/opt/homebrew/Cellar/leptonica" ]]; then
        LEPT_VERSION=$(ls -1 /opt/homebrew/Cellar/leptonica | head -1)
        if [[ -n "$LEPT_VERSION" ]]; then
            BLD_LINKER="$BLD_LINKER -L/opt/homebrew/Cellar/leptonica/$LEPT_VERSION/lib"
        fi
    fi
    
    if [[ -d "/opt/homebrew/Cellar/tesseract" ]]; then
        TESS_VERSION=$(ls -1 /opt/homebrew/Cellar/tesseract | head -1)
        if [[ -n "$TESS_VERSION" ]]; then
            BLD_LINKER="$BLD_LINKER -L/opt/homebrew/Cellar/tesseract/$TESS_VERSION/lib"
        fi
    fi
    
    # Also add homebrew lib path as fallback
    if [[ -d "/opt/homebrew/lib" ]]; then
        BLD_LINKER="$BLD_LINKER -L/opt/homebrew/lib"
    elif [[ -d "/usr/local/lib" ]]; then
        BLD_LINKER="$BLD_LINKER -L/usr/local/lib"
    fi
    
    BLD_LINKER="$BLD_LINKER -lswscale -lavutil -pthread -lavformat -lavcodec -lavfilter -lleptonica -ltesseract"
fi

echo "Running pre-build script..."
./pre-build.sh
echo "Trying to compile..."

# Check for cargo
echo "Checking for cargo..."
if ! [ -x "$(command -v cargo)" ]; then
    echo 'Error: cargo is not installed.' >&2
    exit 1
fi

# Check rust version
rustc_version="$(rustc --version)"
semver=( ${rustc_version//./ } )
version="${semver[1]}.${semver[2]}.${semver[3]}"
MSRV="1.87.0"
if [ "$(printf '%s\n' "$MSRV" "$version" | sort -V | head -n1)" = "$MSRV" ]; then
    echo "rustc >= MSRV(${MSRV})"
else
    echo "Minimum supported rust version(MSRV) is ${MSRV}, please upgrade rust"
    exit 1
fi

echo "Building rust files..."
(cd ../src/rust && CARGO_TARGET_DIR=../../mac/rust cargo build $RUST_PROFILE $RUST_FEATURES) || { echo "Failed building Rust components." ; exit 1; }

# Copy the Rust library
cp $RUST_LIB ./libccx_rust.a

# Add Rust library to linker flags
BLD_LINKER="$BLD_LINKER ./libccx_rust.a"

echo "Building ccextractor"
out=$((LC_ALL=C gcc $BLD_FLAGS $BLD_INCLUDE -o ccextractor $BLD_SOURCES $BLD_LINKER) 2>&1)
res=$?

# Handle common error cases
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

if [[ $res -ne 0 ]]; then  # Unknown error
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
