#!/bin/bash

set -x
set -e

# store the path of where the script is
OLD_CWD=$(readlink -f .)

# store repo root as variable
REPO_ROOT=$(dirname $OLD_CWD)

# Make a temp directory for building stuff which will be cleaned automatically
BUILD_DIR="$OLD_CWD/temp"

# Check if temp directory exist, and if so then remove contents from it
# if not then create temp directory
if [ -d "$BUILD_DIR" ]; then
	rm -r "$BUILD_DIR/*" | true
else
	mkdir -p "$BUILD_DIR"
fi

# make sure to clean up build dir, even if errors occur
cleanup() {
	if [ -d "$BUILD_DIR" ]; then
		rm -rf "$BUILD_DIR"
	fi
}

# Automatically trigger Cleanup function
trap cleanup EXIT

# switch to build dir
pushd "$BUILD_DIR"

# configure build files with CMake
# we need to explicitly set the install prefix, as CMake's default is /usr/local for some reason...
cmake "$REPO_ROOT/src"

# build project and install files into AppDir
make -j$(nproc) ENABLE_OCR=yes

# download linuxdeploy tool
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

# make them executable
chmod +x linuxdeploy*.AppImage

# Create AppDir
mkdir -p "$BUILD_DIR/AppDir"

# Link of CCExtractor image of any of these resolution(8x8, 16x16, 20x20, 22x22, 24x24, 28x28, 32x32, 36x36, 42x42,
# 48x48, 64x64, 72x72, 96x96, 128x128, 160x160, 192x192, 256x256, 384x384, 480x480, 512x512) in png extension
PNG_LINK="https://ccextractor.org/images/ccextractor.png"

# Download the image and put it in AppDir
wget "$PNG_LINK" -P AppDir

# now, build AppImage using linuxdeploy
./linuxdeploy-x86_64.AppImage --appdir=AppDir -e ccextractor --create-desktop-file --output appimage -i AppDir/ccextractor.png

# Move resulted AppImage binary to base directory
mv ccextractor*.AppImage "$OLD_CWD"
