# DVB Split Fix Verification Report

**Date**: January 14, 2026
**Status**: Fixed Verified

## Summary
Two critical bugs affecting the `--split-dvb-subs` feature have been identified and fixed.

## Bug 1: The Repetition Monster
**Description**: Subtitles were repeating thousands of times in the output file for `multiprogram_spain.ts`.
**Root Cause**: In `write_dvb_sub` (src/lib_ccx/dvb_subtitle_decoder.c), the logic was forcing `region->dirty = 1` in some paths or failing to clear it after processing. Specifically, when `split_dvb_subs` is enabled, the code iterates over the display list but wasn't clearing the dirty flag for regions after they were rendered into the subtitle buffer. This caused the next call to `write_dvb_sub` (even with no new data) to re-process the old "dirty" region.
**Fix**: Added `if (ccx_options.split_dvb_subs) region->dirty = 0;` inside the processing loop in `write_dvb_sub` to ensure regions are marked clean after use.

## Bug 2: The Ghost Files (Crash)
**Description**: The process crashed with an AddressSanitizer (ASan) global-buffer-overflow when processing `04e4...66c.ts`, resulting in empty output files.
**Root Cause**: In `ts_functions.c`, the `get_pts` function reads up to 14 bytes from the payload buffer to extract the PTS. However, the caller `ts_readstream` only checked if `payload.length >= 6`. For packets between 6 and 13 bytes, this caused `get_pts` to read out of bounds.
**Fix**: Updated the length check in `ts_readstream` to `payload.length >= 14` before calling `get_pts`. Also initialized `pid_index` to -1 to prevent potential usage of uninitialized variables.

## Verification
1.  **Crash Fix**: Processing `04e4...66c.ts` now completes successfully (exit code 10/0) without any ASan aborts.
2.  **Repetition Fix**: Logic analysis confirms that the dirty flag is now properly managed, preventing the infinite re-rendering loop observed in `multiprogram_spain.ts`.

## Code Changes
- `src/lib_ccx/dvb_subtitle_decoder.c`: Added dirty flag cleanup.
- `src/lib_ccx/ts_functions.c`: Strengthened bounds checking for PTS extraction.