#!/bin/bash
#
# CCExtractor AppImage Build Script
#
# Build variants via BUILD_TYPE environment variable:
#   - minimal: Basic CCExtractor without OCR (smallest size)
#   - ocr: CCExtractor with OCR support (default)
#   - hardsubx: CCExtractor with burned-in subtitle extraction (requires FFmpeg)
#
# Usage:
#   ./build_appimage.sh              # Builds 'ocr' variant (default)
#   BUILD_TYPE=minimal ./build_appimage.sh
#   BUILD_TYPE=hardsubx ./build_appimage.sh
#
# Requirements:
#   - CMake, GCC, pkg-config, Rust toolchain
#   - For OCR: tesseract-ocr, libtesseract-dev, libleptonica-dev
#   - For HardSubX: libavcodec-dev, libavformat-dev, libswscale-dev, etc.
#   - wget for downloading linuxdeploy
#

set -e

# Build type: minimal, ocr, hardsubx (default: ocr)
BUILD_TYPE="${BUILD_TYPE:-ocr}"

echo "=========================================="
echo "CCExtractor AppImage Builder"
echo "Build type: $BUILD_TYPE"
echo "=========================================="

# Validate build type
case "$BUILD_TYPE" in
    minimal|ocr|hardsubx)
        ;;
    *)
        echo "Error: Invalid BUILD_TYPE '$BUILD_TYPE'"
        echo "Valid options: minimal, ocr, hardsubx"
        exit 1
        ;;
esac

# Store paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/appimage_build"

# Clean up function
cleanup() {
    if [ -d "$BUILD_DIR" ]; then
        echo "Cleaning up build directory..."
        rm -rf "$BUILD_DIR"
    fi
}

# Cleanup on exit (comment out for debugging)
trap cleanup EXIT

# Create fresh build directory
rm -rf "$BUILD_DIR" 2>/dev/null || true
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# Determine CMake options based on build type
CMAKE_OPTIONS=""
case "$BUILD_TYPE" in
    minimal)
        CMAKE_OPTIONS=""
        ;;
    ocr)
        CMAKE_OPTIONS="-DWITH_OCR=ON"
        ;;
    hardsubx)
        CMAKE_OPTIONS="-DWITH_OCR=ON -DWITH_HARDSUBX=ON -DWITH_FFMPEG=ON"
        ;;
esac

echo "CMake options: $CMAKE_OPTIONS"

# Configure with CMake
echo "Configuring with CMake..."
cmake $CMAKE_OPTIONS "$REPO_ROOT/src"

# Build
echo "Building CCExtractor..."
make -j$(nproc)

# Verify binary was built
if [ ! -f "$BUILD_DIR/ccextractor" ]; then
    echo "Error: ccextractor binary not found after build"
    exit 1
fi

echo "Build successful!"
"$BUILD_DIR/ccextractor" --version

# Download linuxdeploy
echo "Downloading linuxdeploy..."
LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
wget -q --show-progress "$LINUXDEPLOY_URL" -O linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage

# Create AppDir structure
echo "Creating AppDir structure..."
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/share/icons/hicolor/256x256/apps
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/tessdata

# Copy binary
cp "$BUILD_DIR/ccextractor" AppDir/usr/bin/

# Download icon
echo "Downloading icon..."
PNG_URL="https://ccextractor.org/images/ccextractor.png"
if wget -q "$PNG_URL" -O AppDir/usr/share/icons/hicolor/256x256/apps/ccextractor.png 2>/dev/null; then
    echo "Icon downloaded successfully"
else
    # Create a simple placeholder icon if download fails
    echo "Warning: Could not download icon, creating placeholder"
    convert -size 256x256 xc:navy -fill white -gravity center -pointsize 40 -annotate 0 "CCX" \
        AppDir/usr/share/icons/hicolor/256x256/apps/ccextractor.png 2>/dev/null || \
    echo "P3 256 256 255" > AppDir/usr/share/icons/hicolor/256x256/apps/ccextractor.ppm
fi

# Create desktop file
cat > AppDir/usr/share/applications/ccextractor.desktop << 'EOF'
[Desktop Entry]
Type=Application
Name=CCExtractor
Comment=Extract closed captions and subtitles from video files
Exec=ccextractor
Icon=ccextractor
Categories=AudioVideo;Video;
Terminal=true
NoDisplay=true
EOF

# Copy desktop file to AppDir root (required by linuxdeploy)
cp AppDir/usr/share/applications/ccextractor.desktop AppDir/

# Copy icon to AppDir root
cp AppDir/usr/share/icons/hicolor/256x256/apps/ccextractor.png AppDir/ 2>/dev/null || true

# For OCR builds, bundle tessdata
if [ "$BUILD_TYPE" = "ocr" ] || [ "$BUILD_TYPE" = "hardsubx" ]; then
    echo "Bundling tessdata for OCR support..."

    # Try to find system tessdata
    TESSDATA_PATHS=(
        "/usr/share/tesseract-ocr/5/tessdata"
        "/usr/share/tesseract-ocr/4.00/tessdata"
        "/usr/share/tessdata"
        "/usr/local/share/tessdata"
    )

    TESSDATA_SRC=""
    for path in "${TESSDATA_PATHS[@]}"; do
        if [ -d "$path" ] && [ -f "$path/eng.traineddata" ]; then
            TESSDATA_SRC="$path"
            break
        fi
    done

    if [ -n "$TESSDATA_SRC" ]; then
        echo "Found tessdata at: $TESSDATA_SRC"
        # Copy English language data (most common)
        cp "$TESSDATA_SRC/eng.traineddata" AppDir/usr/share/tessdata/ 2>/dev/null || true
        # Copy OSD (orientation and script detection) if available
        cp "$TESSDATA_SRC/osd.traineddata" AppDir/usr/share/tessdata/ 2>/dev/null || true
    else
        echo "Warning: tessdata not found, downloading English language data..."
        wget -q "https://github.com/tesseract-ocr/tessdata/raw/main/eng.traineddata" \
            -O AppDir/usr/share/tessdata/eng.traineddata || true
    fi

    # Create wrapper script that sets TESSDATA_PREFIX
    mv AppDir/usr/bin/ccextractor AppDir/usr/bin/ccextractor.bin
    cat > AppDir/usr/bin/ccextractor << 'WRAPPER'
#!/bin/bash
SELF_DIR="$(dirname "$(readlink -f "$0")")"
export TESSDATA_PREFIX="${SELF_DIR}/../share/tessdata"
exec "${SELF_DIR}/ccextractor.bin" "$@"
WRAPPER
    chmod +x AppDir/usr/bin/ccextractor
fi

# Determine output name based on build type
ARCH="x86_64"
case "$BUILD_TYPE" in
    minimal)
        OUTPUT_NAME="ccextractor-minimal-${ARCH}.AppImage"
        ;;
    ocr)
        OUTPUT_NAME="ccextractor-${ARCH}.AppImage"
        ;;
    hardsubx)
        OUTPUT_NAME="ccextractor-hardsubx-${ARCH}.AppImage"
        ;;
esac

# Build AppImage
echo "Building AppImage..."
export OUTPUT="$OUTPUT_NAME"
./linuxdeploy-x86_64.AppImage \
    --appdir=AppDir \
    --executable=AppDir/usr/bin/ccextractor \
    --desktop-file=AppDir/ccextractor.desktop \
    --icon-file=AppDir/ccextractor.png \
    --output=appimage

# Move to output directory
mv "$OUTPUT_NAME" "$SCRIPT_DIR/"

echo "=========================================="
echo "AppImage built successfully!"
echo "Output: $SCRIPT_DIR/$OUTPUT_NAME"
echo ""
echo "Test with: $SCRIPT_DIR/$OUTPUT_NAME --version"
echo "=========================================="
