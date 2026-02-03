#!/bin/bash
# WebVTT X-TIMESTAMP-MAP Test Script
# Tests that CCExtractor always includes X-TIMESTAMP-MAP header in WebVTT output

set -e

CCEXTRACTOR="${1:-./ccextractor}"
TEST_DIR="$(dirname "$0")"
OUTPUT_DIR="$TEST_DIR/output"
PASSED=0
FAILED=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo "=========================================="
echo "WebVTT X-TIMESTAMP-MAP Test Suite"
echo "=========================================="
echo "CCExtractor: $CCEXTRACTOR"
echo ""

# Check if ccextractor exists
if [ ! -f "$CCEXTRACTOR" ]; then
    echo -e "${RED}ERROR: CCExtractor not found at $CCEXTRACTOR${NC}"
    echo "Usage: $0 /path/to/ccextractor"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR"/*.vtt

# Function to run a test
run_test() {
    local test_name="$1"
    local input_file="$2"
    local extra_args="$3"
    local output_file="$OUTPUT_DIR/${test_name}.vtt"
    
    echo -n "Test: $test_name... "
    
    # Run CCExtractor
    "$CCEXTRACTOR" "$input_file" -out=webvtt -o "$output_file" $extra_args >/dev/null 2>&1 || true
    
    # Check if output file exists
    if [ ! -f "$output_file" ]; then
        echo -e "${RED}FAILED${NC} (no output file)"
        ((FAILED++))
        return
    fi
    
    # Check for WEBVTT header on line 1
    line1=$(head -1 "$output_file")
    if [ "$line1" != "WEBVTT" ]; then
        echo -e "${RED}FAILED${NC} (line 1 is not WEBVTT)"
        ((FAILED++))
        return
    fi
    
    # Check for X-TIMESTAMP-MAP on line 2
    line2=$(sed -n '2p' "$output_file")
    if [[ ! "$line2" =~ ^X-TIMESTAMP-MAP= ]]; then
        echo -e "${RED}FAILED${NC} (line 2 missing X-TIMESTAMP-MAP)"
        ((FAILED++))
        return
    fi
    
    # Check for exactly one X-TIMESTAMP-MAP header
    count=$(grep -c "X-TIMESTAMP-MAP" "$output_file")
    if [ "$count" -ne 1 ]; then
        echo -e "${RED}FAILED${NC} (found $count X-TIMESTAMP-MAP headers, expected 1)"
        ((FAILED++))
        return
    fi
    
    echo -e "${GREEN}PASSED${NC} [$line2]"
    ((PASSED++))
}

# Create a minimal empty TS file for testing (188 bytes * 100 packets)
echo "Creating test files..."
dd if=/dev/zero of="$TEST_DIR/empty_test.ts" bs=188 count=100 2>/dev/null

# Test 1: Empty TS file (no subtitles)
run_test "empty_ts_no_subs" "$TEST_DIR/empty_test.ts" ""

# Test 2: Empty TS with invalid datapid
run_test "invalid_datapid" "$TEST_DIR/empty_test.ts" "--datapid 9999"

# Test 3: Real sample if available
if [ -f "$TEST_DIR/sample.ts" ]; then
    run_test "real_sample" "$TEST_DIR/sample.ts" ""
fi

# Cleanup
rm -f "$TEST_DIR/empty_test.ts"

echo ""
echo "=========================================="
echo "Results: $PASSED passed, $FAILED failed"
echo "=========================================="

if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
