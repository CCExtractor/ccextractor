#!/bin/bash
set -e
mkdir -p build_cmake
cd build_cmake
cmake ../src -DCMAKE_BUILD_TYPE=Debug
make -j4
echo "Build complete."
