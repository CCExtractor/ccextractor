#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CCX="$SCRIPT_DIR/../../../mac/ccextractor"
SAMPLE="$SCRIPT_DIR/bframe_pts_regression_small.ts"
EXPECTED="$SCRIPT_DIR/bframe_pts_expected.srt"
OUTPUT="$SCRIPT_DIR/out.srt"

$CCX $SAMPLE -o $OUTPUT > /dev/null 2>&1

diff $EXPECTED $OUTPUT > /dev/null 2>&1

if [ $? -eq 0 ]; then
    echo "PASS"
    exit 0
else
    echo "FAIL"
    exit 1
fi
