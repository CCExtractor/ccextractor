# CEA-608 Test Corpus — First Pass

Hand-crafted SCC (Scenarist Closed Caption) test files for validating
CCExtractor's CEA-608 subtitle decoding pipeline (process_cc_data /
ccxr_process_cc_data).

## Test Files

| File | Mode | What it tests |
|------|------|---------------|
| popon_basic.scc | Pop-on | ENM, RCL, PAC row positioning, EDM/EOC buffer flip, multi-line captions, >> speaker change |
| rollup_basic.scc | Roll-up | RU2 to RU3 mode switch, CR carriage-return scrolling, base-row positioning |
| special_chars.scc | Pop-on | Special characters (music note, registered, trademark), extended characters (accented E, I, A), replacement semantics |

## Running

    ./run_tests.sh                        # uses ../../build/ccextractor
    ./run_tests.sh /path/to/ccextractor   # custom binary path

The runner extracts each .scc to SRT, trims trailing whitespace, and
diffs against the .expected.srt reference files.

## How the SCC files were created

generate_scc.py builds each file programmatically using the CEA-608
odd-parity table and correct control-code sequences. All control codes
are doubled for redundancy per broadcast convention.

Reference: http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_FORMAT.HTML
