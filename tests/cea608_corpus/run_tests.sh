#!/bin/bash
CCEXTRACTOR="${1:-../../build/ccextractor}"
PASS=0
FAIL=0
TOTAL=0

if [ ! -x "$CCEXTRACTOR" ]; then
    echo "ERROR: ccextractor binary not found at: $CCEXTRACTOR"
    echo "Usage: $0 [path_to_ccextractor]"
    exit 1
fi

for scc in popon_basic.scc rollup_basic.scc special_chars.scc; do
    name="${scc%.scc}"
    expected="${name}.expected.srt"
    actual="${name}.actual.srt"
    TOTAL=$((TOTAL + 1))

    if [ ! -f "$expected" ]; then
        echo "SKIP  $name — missing $expected"
        continue
    fi

    "$CCEXTRACTOR" "$scc" -o "$actual" > /dev/null 2>&1
    sed 's/[[:space:]]*$//' "$actual" > "$actual.clean"
    mv "$actual.clean" "$actual"

    if diff -q "$expected" "$actual" > /dev/null 2>&1; then
        echo "PASS  $name"
        PASS=$((PASS + 1))
        rm -f "$actual"
    else
        echo "FAIL  $name"
        diff -u "$expected" "$actual"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "Results: $PASS/$TOTAL passed, $FAIL failed"
[ $FAIL -eq 0 ] && exit 0 || exit 1
