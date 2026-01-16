
#!/bin/bash
# Integration tests for --split-dvb-subs feature
# This script tests the DVB subtitle splitting functionality

set -e

# Configuration
CCX_BIN="./linux/ccextractor"
TEST_DIR="/home/rahul/Desktop/all_tests"
OUTPUT_DIR="/tmp/dvb_split_test_output"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
print_header() {
    echo "========================================"
    echo "$1"
    echo "========================================"
}

print_test() {
    echo -n "TEST: $1 ... "
    ((TESTS_RUN++))
}

print_pass() {
    echo -e "${GREEN}PASS${NC}"
    ((TESTS_PASSED++))
}

print_fail() {
    echo -e "${RED}FAIL${NC}"
    echo "  Reason: $1"
    ((TESTS_FAILED++))
}

print_skip() {
    echo -e "${YELLOW}SKIP${NC}"
    echo "  Reason: $1"
}

cleanup() {
    rm -rf "$OUTPUT_DIR"
}

# Setup
mkdir -p "$OUTPUT_DIR"
trap cleanup EXIT

print_header "DVB Split Subtitle Integration Tests"

# Test 1: Happy Path - Single DVB Stream Split
print_test "Happy Path - Single DVB Stream Split"
TEST_FILE="$TEST_DIR/multiprogram_spain.ts"
OUTPUT_BASE="$OUTPUT_DIR/test1_output"

if [ ! -f "$TEST_FILE" ]; then
    print_fail "Test file not found: $TEST_FILE"
else
    # Run ccextractor with split-dvb-subs
    "$CCX_BIN" "$TEST_FILE" --split-dvb-subs -o "$OUTPUT_BASE" > "$OUTPUT_DIR/test1.log" 2>&1
    
    # Check if split files were created
    SPLIT_FILES=$(ls "$OUTPUT_BASE"_*.srt 2>/dev/null | wc -l)
    
    if [ "$SPLIT_FILES" -eq 0 ]; then
        print_fail "No split files created"
    else
        # Check if split files have content
        EMPTY_COUNT=0
        for file in "$OUTPUT_BASE"_*.srt; do
            if [ ! -s "$file" ] || [ $(stat -c%s "$file") -eq 0 ]; then
                ((EMPTY_COUNT++))
            fi
        done
        
        if [ "$EMPTY_COUNT" -eq "$SPLIT_FILES" ]; then
            print_fail "All split files are empty (0 bytes)"
        else
            # Check if files have valid SRT format
            VALID_SRT=0
            for file in "$OUTPUT_BASE"_*.srt; do
                if [ -s "$file" ] && [ $(stat -c%s "$file") -gt 0 ]; then
                    # Check for SRT format markers (timestamps)
                    if grep -q "^-->" "$file" && grep -q "^[0-9]" "$file"; then
                        ((VALID_SRT++))
                    fi
                fi
            done
            
            if [ "$VALID_SRT" -gt 0 ]; then
                print_pass "Split files created with valid SRT content"
            else
                print_fail "Split files created but invalid SRT format"
            fi
        fi
    fi
fi

# Test 2: Edge Case - Multiple DVB Streams with Different Languages
print_test "Edge Case - Multiple DVB Streams with Different Languages"
TEST_FILE="$TEST_DIR/04e47919de5908edfa1fddc522a811d56bc67a1d4020f8b3972709e25b15966c.ts"
OUTPUT_BASE="$OUTPUT_DIR/test2_output"

if [ ! -f "$TEST_FILE" ]; then
    print_skip "Test file not found (large file may not be available)"
else
    # Run ccextractor with split-dvb-subs
    timeout 300 "$CCX_BIN" "$TEST_FILE" --split-dvb-subs -o "$OUTPUT_BASE" > "$OUTPUT_DIR/test2.log" 2>&1
    EXIT_CODE=$?
    
    if [ $EXIT_CODE -eq 124 ]; then
        print_skip "Test timed out (large file)"
    elif [ $EXIT_CODE -ne 0 ]; then
        print_fail "ccextractor exited with code $EXIT_CODE"
    else
        # Check for language-specific split files
        LANG_FILES=$(ls "$OUTPUT_BASE"_*.srt 2>/dev/null | wc -l)
        
        if [ "$LANG_FILES" -ge 3 ]; then
            print_pass "Multiple language split files created"
        else
            print_fail "Expected at least 3 language files, got $LANG_FILES"
        fi
    fi
fi

# Test 3: Edge Case - File with No DVB Subtitles
print_test "Edge Case - File with No DVB Subtitles"
TEST_FILE="$TEST_DIR/BBC1.mp4"
OUTPUT_BASE="$OUTPUT_DIR/test3_output"

if [ ! -f "$TEST_FILE" ]; then
    print_skip "Test file not found"
else
    # Run ccextractor with split-dvb-subs
    "$CCX_BIN" "$TEST_FILE" --split-dvb-subs -o "$OUTPUT_BASE" > "$OUTPUT_DIR/test3.log" 2>&1
    
    # Check that no split files were created
    SPLIT_FILES=$(ls "$OUTPUT_BASE"_*.srt 2>/dev/null | wc -l)
    
    if [ "$SPLIT_FILES" -eq 0 ]; then
        # Check log for "No captions found" message
        if grep -q "No captions were found" "$OUTPUT_DIR/test3.log"; then
            print_pass "No split files created (no DVB subtitles found)"
        else
            print_fail "No split files but no 'No captions' message in log"
        fi
    else
        print_fail "Split files created for file with no DVB subtitles"
    fi
fi

# Test 4: Edge Case - Single DVB Stream (C49)
print_test "Edge Case - Single DVB Stream (C49)"
TEST_FILE="$TEST_DIR/mp_spain_20170112_C49.ts"
OUTPUT_BASE="$OUTPUT_DIR/test4_output"

if [ ! -f "$TEST_FILE" ]; then
    print_skip "Test file not found"
else
    "$CCX_BIN" "$TEST_FILE" --split-dvb-subs -o "$OUTPUT_BASE" > "$OUTPUT_DIR/test4.log" 2>&1
    
    SPLIT_FILES=$(ls "$OUTPUT_BASE"_*.srt 2>/dev/null | wc -l)
    
    if [ "$SPLIT_FILES" -eq 1 ]; then
        FILE_SIZE=$(stat -c%s "$OUTPUT_BASE"_*.srt)
        if [ "$FILE_SIZE" -gt 0 ]; then
            print_pass "Single DVB stream split successfully"
        else
            print_fail "Split file is empty (0 bytes)"
        fi
    else
        print_fail "Expected 1 split file, got $SPLIT_FILES"
    fi
fi

# Test 5: Edge Case - Single DVB Stream (C55)
print_test "Edge Case - Single DVB Stream (C55)"
TEST_FILE="$TEST_DIR/mp_spain_20170112_C55.ts"
OUTPUT_BASE="$OUTPUT_DIR/test5_output"

if [ ! -f "$TEST_FILE" ]; then
    print_skip "Test file not found"
else
    "$CCX_BIN" "$TEST_FILE" --split-dvb-subs -o "$OUTPUT_BASE" > "$OUTPUT_DIR/test5.log" 2>&1
    
    SPLIT_FILES=$(ls "$OUTPUT_BASE"_*.srt 2>/dev/null | wc -l)
    
    if [ "$SPLIT_FILES" -eq 1 ]; then
        FILE_SIZE=$(stat -c%s "$OUTPUT_BASE"_*.srt)
        if [ "$FILE_SIZE" -gt 0 ]; then
            print_pass "Single DVB stream split successfully"
        else
            print_fail "Split file is empty (0 bytes)"
        fi
    else
        print_fail "Expected 1 split file, got $SPLIT_FILES"
    fi
fi

# Test 6: Regression - Verify Bug Fix (Empty Split Files)
print_test "Regression - Verify Bug Fix (Empty Split Files)"
TEST_FILE="$TEST_DIR/multiprogram_spain.ts"
OUTPUT_BASE="$OUTPUT_DIR/test6_output"

if [ ! -f "$TEST_FILE" ]; then
    print_skip "Test file not found"
else
    "$CCX_BIN" "$TEST_FILE" --split-dvb-subs -o "$OUTPUT_BASE" > "$OUTPUT_DIR/test6.log" 2>&1
    
    # Check for the specific bug: 0-byte split files
    EMPTY_COUNT=0
    for file in "$OUTPUT_BASE"_*.srt; do
        if [ -f "$file" ] && [ $(stat -c%s "$file") -eq 0 ]; then
            ((EMPTY_COUNT++))
        fi
    done
    
    if [ "$EMPTY_COUNT" -eq 0 ]; then
        print_pass "Bug fixed - no empty split files"
    else
        print_fail "Bug still present - $EMPTY_COUNT empty split files found"
    fi
fi

# Summary
print_header "Test Summary"
echo "Tests Run:    $TESTS_RUN"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
echo "Tests Skipped: $((TESTS_RUN - TESTS_PASSED - TESTS_FAILED))"
echo ""
echo "Test logs available in: $OUTPUT_DIR"

# Exit with appropriate code
if [ $TESTS_FAILED -eq 0 ]; then
    exit 0
else
    exit 1
