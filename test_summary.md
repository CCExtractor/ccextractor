# Test Summary (Superpowers: Write)

## Bug Description
The "empty output file" issue during DVB splitting was identified as a filename generation bug. When using the `--split-dvb-subs` option with an output path containing directory components (e.g., `./test_out/file`), CCExtractor failed to construct the output filenames for split streams correctly. Specifically, the `get_basename` function incorrectly truncated the base filename, often resulting in empty strings or stripped directory paths. This caused split files to be written to the current working directory with malformed names (e.g., `_spa_0x006F.srt`) instead of the intended location.

## Fix Implementation
**File**: `src/lib_ccx/utility.c`
**Function**: `get_basename`
**Change**: Rewrote the function to strictly strip the file extension only if it appears *after* the last path separator. This ensures that relative paths (like `./`) and directory structures are preserved in the base filename used for generating split file names.

## Verification
**Test Script**: `run_mass_test.sh` (modified to target `multiprogram_spain.ts` and output to `./test_out`)
**Build**: Recompiled `ccextractor` using CMake.
**Results**:
- **Before Fix**: Output files were missing from `./test_out`. Files appeared in the root directory as `_spa_XXXX.srt`.
- **After Fix**: Split files are correctly generated in `./test_out` with the proper prefix:
    - `multiprogram_spain_split_spa_0x006F.srt` (1600 bytes)
    - `multiprogram_spain_split_spa_0x00D3.srt` (1701 bytes)
    - `multiprogram_spain_split_spa_0x0137.srt` (1602 bytes)
    - `multiprogram_spain_split_spa_0x03F3.srt` (2323 bytes)
    - `multiprogram_spain_split_spa_0x05E7.srt` (2488 bytes)

## Conclusion
The bug is **FIXED**. DVB splitting now respects the output directory and filename prefix specified by the `-o` parameter.
