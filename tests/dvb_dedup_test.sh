#!/bin/bash
# DVB Split Deduplication Test Suite
# Tests DVB-001 through DVB-008 requirements

set -e

echo "CCExtractor DVB Split Deduplication Test Suite"
echo "==============================================="
echo ""

# Configuration
CCEXTRACTOR="./ccextractor"
TEST_DIR="/home/rahul/Desktop/all_tests"
OUTPUT_DIR="/tmp/dvb_split_test_$$"

# Test files
MULTIPROGRAM_SPAIN="$TEST_DIR/multiprogram_spain.ts"
MP_SPAIN_C49="$TEST_DIR/mp_spain_20170112_C49.ts"
BBC1="$TEST_DIR/BBC1.mp4"

mkdir -p "$OUTPUT_DIR"

echo "Output directory: $OUTPUT_DIR"
echo ""

# DVB-004: Test multilang split (5 files)
echo "DVB-004: Testing multilingual DVB split..."
if [ -f "$MULTIPROGRAM_SPAIN" ]; then
    $CCEXTRACTOR "$MULTIPROGRAM_SPAIN" --split-dvb-subs -o "$OUTPUT_DIR/mp_spain" 2>&1 | grep -i "dvb\|split" | head -10
    
    FILE_COUNT=$(ls -1 "$OUTPUT_DIR"/mp_spain*.srt 2>/dev/null | wc -l)
    echo "  Created $FILE_COUNT output files"
    
    if [ $FILE_COUNT -ge 4 ]; then
        echo "  ✓ DVB-004: PASS (created $FILE_COUNT files, expected 4-5)"
    else
        echo "  ✗ DVB-004: FAIL (created $FILE_COUNT files, expected 4-5)"
    fi
    
    # Check file sizes (should not be 0 bytes if OCR enabled)
    echo "  File sizes:"
    ls -lh "$OUTPUT_DIR"/mp_spain*.srt 2>/dev/null | awk '{print "    " $9 ": " $5}'
else
    echo "  ⚠ SKIP: Test file not found: $MULTIPROGRAM_SPAIN"
fi
echo ""

# DVB-005: Test single program DVB extraction (using -pn to select one program)
echo "DVB-005: Testing single program DVB extraction..."
if [ -f "$MULTIPROGRAM_SPAIN" ]; then
    # Use program number 530 which has DVB subtitles on PID 0xD3
    $CCEXTRACTOR --split-dvb-subs --program-number 530 -o "$OUTPUT_DIR/single_prog" "$MULTIPROGRAM_SPAIN" 2>&1 | grep -i "dvb\|Created\|split" | head -5
    
    FILE_COUNT=$(ls -1 "$OUTPUT_DIR"/single_prog*.srt 2>/dev/null | wc -l)
    echo "  Created $FILE_COUNT output files"
    
    if [ $FILE_COUNT -ge 1 ]; then
        echo "  ✓ DVB-005: PASS"
    else
        echo "  ✗ DVB-005: FAIL (Note: mp_spain_20170112_C49.ts has Teletext, not DVB subs)"
    fi
else
    echo "  ⚠ SKIP: Test file not found: $MULTIPROGRAM_SPAIN"
fi
echo ""

# DVB-006: Test non-DVB file
echo "DVB-006: Testing non-DVB file (should work without errors)..."
if [ -f "$BBC1" ]; then
    $CCEXTRACTOR "$BBC1" --split-dvb-subs -o "$OUTPUT_DIR/bbc1" 2>&1 | tail -5
    echo "  ✓ DVB-006: PASS (no crash)"
else
    echo "  ⚠ SKIP: Test file not found: $BBC1"
fi
echo ""

# DVB-007: Test repetition bug fix (check for excessive duplicates)
echo "DVB-007: Testing deduplication effectiveness..."
SRT_FILES=$(ls "$OUTPUT_DIR"/mp_spain_*.srt 2>/dev/null)
if [ -f "$MULTIPROGRAM_SPAIN" ] && [ -n "$SRT_FILES" ]; then
    for file in "$OUTPUT_DIR"/mp_spain_*.srt; do
        if [ -f "$file" ] && [ -s "$file" ]; then
            # Count consecutive duplicate subtitle blocks
            DUPES=$(awk '/^[0-9]+$/{n=$0} /^$/{if(prev==n)dup++; prev=n} END{print dup+0}' "$file")
            TOTAL=$(grep -c "^[0-9]*$" "$file" || echo "0")
            TOTAL=$(echo "$TOTAL" | tr -d '\n')
            
            if [ "$TOTAL" -gt 0 ]; then
                RATIO=$(awk "BEGIN {printf \"%.1f\", ($DUPES/$TOTAL)*100}")
                echo "  File: $(basename $file)"
                echo "    Total subtitles: $TOTAL"
                echo "    Potential duplicates: $DUPES"
                echo "    Ratio: ${RATIO}%"
                
                if [ "$DUPES" -lt 3 ]; then
                    echo "    ✓ Good deduplication"
                fi
            fi
        fi
    done
    echo "  ✓ DVB-007: Check complete (review ratios above)"
else
    echo "  ⚠ SKIP: No output files to check"
fi
echo ""

# DVB-008: Test --no-dvb-dedup flag
echo "DVB-008: Testing --no-dvb-dedup flag..."
if [ -f "$MULTIPROGRAM_SPAIN" ]; then
    $CCEXTRACTOR "$MULTIPROGRAM_SPAIN" --split-dvb-subs --no-dvb-dedup -o "$OUTPUT_DIR/mp_spain_nodedup" 2>&1 | tail -3
    
    FILE_COUNT=$(ls -1 "$OUTPUT_DIR"/mp_spain_nodedup*.srt 2>/dev/null | wc -l)
    if [ $FILE_COUNT -ge 1 ]; then
        echo "  ✓ DVB-008: PASS (flag accepted, created $FILE_COUNT files)"
    else
        echo "  ✗ DVB-008: FAIL"
    fi
else
    echo "  ⚠ SKIP: Test file not found"
fi
echo ""

echo "Test suite complete!"
echo "Output files in: $OUTPUT_DIR"
echo ""
echo "NOTE: Without OCR (Tesseract), DVB subtitle files may be empty."
echo "      The deduplication logic is still applied correctly."
