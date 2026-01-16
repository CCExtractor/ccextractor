# Bug Detection Report: DVB Split Feature

**Date**: January 13, 2026
**Status**: Two Critical Bugs Confirmed
**Method**: Ralph Resilience Engine (Reproduction Track)

## Bug 1: The Repetition Monster
**Severity**: Critical
**Description**: When using `--split-dvb-subs` on `multiprogram_spain.ts`, a single subtitle line is repeated thousands of times in the output file, rendering it unusable.
**Reproduction**:
```bash
./ccextractor multiprogram_spain.ts --split-dvb-subs -o output
```
**Evidence**:
*   File `bug1_test_spa_0x03F3.srt` contains **2659** repetitions of `<font color="#f7f7f7">Laicompania'Yllana cambia</font>`.
**Root Cause Hypothesis**:
*   Logic in `src/lib_ccx/dvb_subtitle_decoder.c` unconditionally forces regions to be "dirty" (updateable) in every loop iteration when split mode is active. This causes the decoder to re-write the same subtitle buffer repeatedly even if no new data arrived.

## Bug 2: The Ghost Files (Empty Output)
**Severity**: High
**Description**: When using `--split-dvb-subs` on `04e4...66c.ts`, the expected subtitle files are created but remain **0 bytes** (empty).
**Reproduction**:
```bash
./ccextractor 04e4...66c.ts --split-dvb-subs -o output
```
**Evidence**:
*   `bug2_test_chi_0x0050.srt`: 0 bytes
*   `bug2_test_chs_0x0052.srt`: 0 bytes
**Additional Findings**:
*   **Crash Detected**: The process aborted with `AddressSanitizer: global-buffer-overflow` in `ts_readstream`.
*   **Location**: `../src/lib_ccx/ts_functions.c:908` reading `tspacket`.
*   This crash might be preventing the buffers from being flushed to disk, resulting in empty files.

## Recommendations
1.  **Fix Bug 1**: Remove the "force dirty" loop in `dvb_subtitle_decoder.c`.
2.  **Fix Bug 2**: Investigate `ts_functions.c` around line 908 to fix the buffer overflow. The empty files are likely a side effect of this crash terminating the program before `flush_buffer()` is called.
