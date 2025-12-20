# CCExtractor Docker Build
#
# Build variants via BUILD_TYPE argument:
#   - minimal: Basic CCExtractor without OCR
#   - ocr: CCExtractor with OCR support (default)
#   - hardsubx: CCExtractor with burned-in subtitle extraction (requires FFmpeg)
#
# Source options via USE_LOCAL_SOURCE argument:
#   - 0 (default): Clone from GitHub (standalone Dockerfile usage)
#   - 1: Use local source (when building from cloned repo)
#
# Build examples:
#
#   # Standalone (just the Dockerfile, clones from GitHub):
#   docker build -t ccextractor docker/
#   docker build --build-arg BUILD_TYPE=hardsubx -t ccextractor docker/
#
#   # From cloned repository (faster, uses local source):
#   docker build --build-arg USE_LOCAL_SOURCE=1 -f docker/Dockerfile -t ccextractor .
#   docker build --build-arg USE_LOCAL_SOURCE=1 --build-arg BUILD_TYPE=minimal -f docker/Dockerfile -t ccextractor .

ARG DEBIAN_VERSION=bookworm-slim

FROM debian:${DEBIAN_VERSION} AS base

FROM base AS builder

# Build arguments
ARG BUILD_TYPE=ocr
ARG USE_LOCAL_SOURCE=0
# BUILD_TYPE: minimal, ocr, hardsubx
# USE_LOCAL_SOURCE: 0 = git clone, 1 = copy local source

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install base build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    curl \
    ca-certificates \
    gcc \
    g++ \
    cmake \
    make \
    pkg-config \
    bash \
    zlib1g-dev \
    libpng-dev \
    libjpeg-dev \
    libssl-dev \
    libfreetype-dev \
    libxml2-dev \
    libcurl4-gnutls-dev \
    clang \
    libclang-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Rust toolchain
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain stable
ENV PATH="/root/.cargo/bin:${PATH}"

# Install OCR dependencies (for ocr and hardsubx builds)
RUN if [ "$BUILD_TYPE" = "ocr" ] || [ "$BUILD_TYPE" = "hardsubx" ]; then \
        apt-get update && apt-get install -y --no-install-recommends \
            tesseract-ocr \
            libtesseract-dev \
            libleptonica-dev \
        && rm -rf /var/lib/apt/lists/*; \
    fi

# Install FFmpeg dependencies (for hardsubx build)
RUN if [ "$BUILD_TYPE" = "hardsubx" ]; then \
        apt-get update && apt-get install -y --no-install-recommends \
            libavcodec-dev \
            libavformat-dev \
            libavutil-dev \
            libswscale-dev \
            libswresample-dev \
            libavfilter-dev \
            libavdevice-dev \
        && rm -rf /var/lib/apt/lists/*; \
    fi

# Build and install GPAC library
WORKDIR /root
RUN git clone -b v2.4.0 --depth 1 https://github.com/gpac/gpac
WORKDIR /root/gpac
RUN ./configure && make -j$(nproc) lib && make install-lib && ldconfig
WORKDIR /root
RUN rm -rf /root/gpac

# Get CCExtractor source (either clone or copy based on USE_LOCAL_SOURCE)
WORKDIR /root
# First, copy local source if provided (will be empty dir if building standalone)
COPY . /root/ccextractor-local/

# Then get source: use local copy if USE_LOCAL_SOURCE=1 and source exists,
# otherwise clone from GitHub
RUN if [ "$USE_LOCAL_SOURCE" = "1" ] && [ -f /root/ccextractor-local/src/ccextractor.c ]; then \
        echo "Using local source"; \
        mv /root/ccextractor-local /root/ccextractor; \
    else \
        echo "Cloning from GitHub"; \
        rm -rf /root/ccextractor-local; \
        git clone --depth 1 https://github.com/CCExtractor/ccextractor.git /root/ccextractor; \
    fi

WORKDIR /root/ccextractor/linux

# Generate build info
RUN ./pre-build.sh

# Build Rust library with appropriate features
RUN if [ "$BUILD_TYPE" = "hardsubx" ]; then \
        cd ../src/rust && \
        CARGO_TARGET_DIR=../../linux/rust cargo build --release --features hardsubx_ocr; \
    else \
        cd ../src/rust && \
        CARGO_TARGET_DIR=../../linux/rust cargo build --release; \
    fi

RUN cp rust/release/libccx_rust.a ./libccx_rust.a

# Compile CCExtractor
RUN if [ "$BUILD_TYPE" = "minimal" ]; then \
        BLD_FLAGS="-std=gnu99 -Wno-write-strings -Wno-pointer-sign -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP -DGPAC_64_BITS"; \
        BLD_INCLUDE="-I../src -I../src/lib_ccx/ -I /usr/include/gpac/ -I../src/thirdparty/libpng -I../src/thirdparty/zlib -I../src/lib_ccx/zvbi -I../src/thirdparty/lib_hash -I../src/thirdparty -I../src/thirdparty/freetype/include"; \
        BLD_LINKER="-lm -Wl,--allow-multiple-definition -lpthread -ldl -lgpac ./libccx_rust.a"; \
    elif [ "$BUILD_TYPE" = "hardsubx" ]; then \
        BLD_FLAGS="-std=gnu99 -Wno-write-strings -Wno-pointer-sign -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -DENABLE_OCR -DENABLE_HARDSUBX -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP -DGPAC_64_BITS"; \
        BLD_INCLUDE="-I../src -I /usr/include/leptonica/ -I /usr/include/tesseract/ -I../src/lib_ccx/ -I /usr/include/gpac/ -I../src/thirdparty/libpng -I../src/thirdparty/zlib -I../src/lib_ccx/zvbi -I../src/thirdparty/lib_hash -I../src/thirdparty -I../src/thirdparty/freetype/include"; \
        BLD_LINKER="-lm -Wl,--allow-multiple-definition -ltesseract -lleptonica -lpthread -ldl -lgpac -lswscale -lavutil -lavformat -lavcodec -lavfilter -lswresample ./libccx_rust.a"; \
    else \
        BLD_FLAGS="-std=gnu99 -Wno-write-strings -Wno-pointer-sign -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -DENABLE_OCR -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP -DGPAC_64_BITS"; \
        BLD_INCLUDE="-I../src -I /usr/include/leptonica/ -I /usr/include/tesseract/ -I../src/lib_ccx/ -I /usr/include/gpac/ -I../src/thirdparty/libpng -I../src/thirdparty/zlib -I../src/lib_ccx/zvbi -I../src/thirdparty/lib_hash -I../src/thirdparty -I../src/thirdparty/freetype/include"; \
        BLD_LINKER="-lm -Wl,--allow-multiple-definition -ltesseract -lleptonica -lpthread -ldl -lgpac ./libccx_rust.a"; \
    fi && \
    SRC_LIBPNG="$(find ../src/thirdparty/libpng/ -name '*.c')" && \
    SRC_ZLIB="$(find ../src/thirdparty/zlib/ -name '*.c')" && \
    SRC_CCX="$(find ../src/lib_ccx/ -name '*.c')" && \
    SRC_GPAC="$(find /usr/include/gpac/ -name '*.c' 2>/dev/null || true)" && \
    SRC_HASH="$(find ../src/thirdparty/lib_hash/ -name '*.c')" && \
    SRC_UTF8PROC="../src/thirdparty/utf8proc/utf8proc.c" && \
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
        ../src/thirdparty/freetype/winfonts/winfnt.c" && \
    BLD_SOURCES="../src/ccextractor.c $SRC_CCX $SRC_GPAC $SRC_ZLIB $SRC_LIBPNG $SRC_HASH $SRC_UTF8PROC $SRC_FREETYPE" && \
    gcc $BLD_FLAGS $BLD_INCLUDE -o ccextractor $BLD_SOURCES $BLD_LINKER

# Copy binary to known location
RUN cp /root/ccextractor/linux/ccextractor /ccextractor

# Final minimal image
FROM base AS final

ARG BUILD_TYPE=ocr

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies based on build type
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpng16-16 \
    libjpeg62-turbo \
    zlib1g \
    libssl3 \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

# OCR runtime dependencies
RUN if [ "$BUILD_TYPE" = "ocr" ] || [ "$BUILD_TYPE" = "hardsubx" ]; then \
        apt-get update && apt-get install -y --no-install-recommends \
            tesseract-ocr \
            liblept5 \
        && rm -rf /var/lib/apt/lists/*; \
    fi

# HardSubX runtime dependencies
RUN if [ "$BUILD_TYPE" = "hardsubx" ]; then \
        apt-get update && apt-get install -y --no-install-recommends \
            libavcodec59 \
            libavformat59 \
            libavutil57 \
            libswscale6 \
            libswresample4 \
            libavfilter8 \
            libavdevice59 \
        && rm -rf /var/lib/apt/lists/*; \
    fi

# Copy GPAC library from builder
COPY --from=builder /usr/local/lib/libgpac.so* /usr/local/lib/

# Update library cache
RUN ldconfig

# Copy CCExtractor binary
COPY --from=builder /ccextractor /ccextractor

ENTRYPOINT ["/ccextractor"]
