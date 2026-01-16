#!/bin/bash
export TESSDATA_PREFIX=/home/rahul/Desktop/ccextractor/tessdata
TEST_DIR="/home/rahul/Desktop/all_tests"
OUT_DIR="/tmp/mass_test"
CC="./linux/ccextractor"

FILES=$(ls -1 $TEST_DIR | grep -E "\.(ts|mpg|mp4)$")

echo "Starting Mass DVB Split Test..."
echo "--------------------------------"

for f in $FILES; do
    echo "Testing File: $f"
    # Get base name without extension
    base=$(basename "$f" | sed 's/\.[^.]*$//')
    
    # Run CCExtractor with split DVB subs
    # Use a timeout to prevent hanging on very large files or loops
    timeout 120 $CC "$TEST_DIR/$f" --split-dvb-subs -o "$OUT_DIR/${base}_split" > "$OUT_DIR/${base}.log" 2>&1
    exit_code=$?
    
    # Check if DVB streams were discovered
    discovered=$(grep "Discovered DVB stream" "$OUT_DIR/${base}.log" | head -n 1)
    if [ -z "$discovered" ]; then
        echo "  - No DVB streams discovered."
    else
        echo "  - DVB Streams Discovered: YES"
        echo "  - Exit Code: $exit_code"
        
        # List generated split files and their sizes
        echo "  - Generated Files:"
        ls -la "$OUT_DIR" | grep "${base}_split" | awk '{print "    " $9 " (" $5 " bytes)"}'
        
        # Check for repetition (Bug 1) if file is non-empty
        # We check if the first two subtitle entries have identical text
        split_file=$(ls "$OUT_DIR" | grep "${base}_split_" | grep ".srt$" | head -n 1)
        if [ ! -z "$split_file" ]; then
            size=$(stat -c%s "$OUT_DIR/$split_file")
            if [ $size -gt 0 ]; then
                # Extremely simple repetition check: head first 20 lines and see if text repeats
                # This is a bit manual but useful for the log
                echo "  - Repetition Check (Top 2 entries):"
                head -n 20 "$OUT_DIR/$split_file" | grep -v "-->" | grep -v "^[0-9]*$" | sort | uniq -c
            fi
        fi
    fi
    echo "--------------------------------"
done
echo "Mass Test Complete. Logs in $OUT_DIR"
