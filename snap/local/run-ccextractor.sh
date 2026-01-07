#!/bin/sh
set -e

export LD_LIBRARY_PATH="$SNAP/usr/lib/x86_64-linux-gnu:\
$SNAP/usr/lib/x86_64-linux-gnu/blas:\
$SNAP/usr/lib/x86_64-linux-gnu/lapack:\
$SNAP/usr/lib/x86_64-linux-gnu/pulseaudio:\
${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

exec "$SNAP/usr/local/bin/ccextractor" "$@"
