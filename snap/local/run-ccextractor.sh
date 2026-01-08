#!/bin/sh
set -e

# Default fallback 
LIB_TRIPLET="x86_64-linux-gnu"

# Detect multiarch directory if present
for d in "$SNAP/usr/lib/"*-linux-gnu; do
    if [ -d "$d" ]; then
        LIB_TRIPLET=$(basename "$d")
        break
    fi
done

export LD_LIBRARY_PATH="$SNAP/usr/lib/$LIB_TRIPLET:\
$SNAP/usr/lib/$LIB_TRIPLET/blas:\
$SNAP/usr/lib/$LIB_TRIPLET/lapack:\
$SNAP/usr/lib/$LIB_TRIPLET/pulseaudio:\
${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

exec "$SNAP/usr/local/bin/ccextractor" "$@"
