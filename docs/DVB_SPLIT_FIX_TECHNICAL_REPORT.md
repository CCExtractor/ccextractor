# Post-Mortem: DVB Split Logic & Buffer Safety Fixes

**Author**: The Scribe (Gemini CLI)
**Date**: January 13, 2026
**Target**: Senior Engineering
**Scope**: `dvb_subtitle_decoder.c` logic lifecycle, `ts_functions.c` memory safety.

---

## 1. Executive Summary
We resolved two critical stability issues in the DVB Split subsystem of CCExtractor:
1.  **Logic Flaw**: A "heuristic" in the DVB decoder forced the `dirty` flag to `1` without ever clearing it, causing the decoder to re-render the same region data for every incoming Transport Stream packet.
2.  **Security/Crash**: An unchecked read in `ts_readstream` caused a global buffer overflow (ASan abort) when parsing malformed or short PES packets.

---

## 2. Deep Dive: The Logic Fix (Bug 1)

### The Problem
The `--split-dvb-subs` mode separates DVB streams. To handle streams that lack explicit `DISPLAY_SEGMENT` commands (like some Arte streams), a heuristic was added:
```c
if (!ctx->display_list) {
    // ... build list from regions ...
    r->dirty = 1; // Forced ON
}
```
This heuristic forces the decoder to render. However, because it modified the region state (which persists) and never reset it, **every subsequent call** to `write_dvb_sub` (triggered by PCR updates or padding packets) saw `dirty=1` and re-rendered the subtitle.

### The Fix
We modified `src/lib_ccx/dvb_subtitle_decoder.c` to:
1.  **Disable the forced dirty flag** (`// r->dirty = 1;`). We now rely on the natural state: `region->dirty` is set to 1 only when new pixel data is actually parsed.
2.  **Enforce cleanup**: Added `region->dirty = 0;` inside the rendering loop to ensure that once a region is rendered, it is marked clean until new data arrives.

**Code Change**:
```c
// src/lib_ccx/dvb_subtitle_decoder.c

// 1. Disable forced dirty
// r->dirty = 1; 

// 2. Clear dirty after render
for (...) {
    if (region->dirty) {
        // ... render ...
    }
    if (ccx_options.split_dvb_subs) region->dirty = 0; // Fix
}
```

**Outcome**:
- **Regression Test**: `mp_spain...C49.ts` produces clean, non-duplicated subtitles.
- **Stress Test**: `multiprogram_spain.ts` (a problematic file) no longer causes infinite internal loops, though stream idiosyncrasies may still trigger frequent updates.

---

## 3. Deep Dive: Buffer Overflow (Bug 2)

### The Problem
In `src/lib_ccx/ts_functions.c`, the parser checked `if (payload.pesstart)` and immediately read 3 bytes from `payload.start` to check the PES prefix.
If `payload.start` pointed to the very end of the buffer (e.g., a truncated packet), this read violated memory safety.

### The Fix
We added a bounds check:
```c
if (payload.pesstart && payload.length >= 6)
```
This ensures we have enough bytes for the standard PES header before reading.

**Outcome**:
- **Before**: Immediate crash with AddressSanitizer.
- **After**: Stable execution (Exit Code 0 or 10).

---

## 4. System Diagram

```mermaid
graph TD
    A[TS Packet] -->|ts_functions.c| B{Length >= 6?};
    B -- No --> C[Skip (Safe)];
    B -- Yes --> D[Parse DVB];
    D -->|Pixel Data| E[Set Dirty=1];
    E --> F[write_dvb_sub];
    F --> G{Dirty?};
    G -- Yes --> H[Render];
    H --> I[Set Dirty=0];
    I --> J[Output];
```