#!/usr/bin/env bash
set -euo pipefail

# build_and_test.sh - safe, repeatable out-of-source CMake build helper
# Usage:
#   ./build_and_test.sh            # uses default options
#   BUILD_DIR=outbuild ./build_and_test.sh   # override build dir
#   CMAKE_OPTS='-DWITH_OCR=OFF' ./build_and_test.sh # override cmake options

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_ROOT"

echo "Repo root: $REPO_ROOT"
BUILD_DIR="${BUILD_DIR:-build}"
CMAKE_OPTS="${CMAKE_OPTS:--DWITH_OCR=ON -DWITH_HARDSUBX=ON -DWITH_FFMPEG=ON}"

# Clean previous build dir (only the one used)
if [ -d "$BUILD_DIR" ]; then
  echo "Removing existing build dir '$BUILD_DIR'"
  rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

echo "Configuring CMake (build dir: $BUILD_DIR)"
# The top-level CMakeLists is under `src/` in this repository
cmake -S src -B "$BUILD_DIR" $CMAKE_OPTS

echo "Building..."
cmake --build "$BUILD_DIR" -- -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 2)"

BIN_PATH="$BUILD_DIR/ccextractor"
if [ -x "$BIN_PATH" ]; then
  echo "Built binary: $BIN_PATH"
  echo "Running: $BIN_PATH --version"
  "$BIN_PATH" --version || true
else
  echo "Warning: built binary not found at $BIN_PATH"
fi

# Run tests if ctest is available
if command -v ctest >/dev/null 2>&1; then
  echo "Running ctest..."
  pushd "$BUILD_DIR" >/dev/null
  ctest --output-on-failure || true
  popd >/dev/null
else
  echo "ctest not found; skipping tests"
fi

echo "Done."