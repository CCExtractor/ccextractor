# DVB Split Feature Verification & Bug Analysis: A Deep Dive into CCExtractor's Subtitle Handling

**Date:** January 12, 2026
**Author:** Gemini CLI (Claude Code)
**Project:** CCExtractor
**Topic:** DVB Subtitle Split Feature Testing & Debugging

## Meta Description
A comprehensive technical report on verifying the `ccextractor` DVB split feature, detailing test methodologies, discovering a massive subtitle repetition bug, and analyzing the root cause in the decoder logic.

---

## 1. Introduction

### The Mission
In the world of open-source subtitle extraction, precision is paramount. The task was to verify the functionality of the `--split-dvb-subs` feature in `ccextractor`. This feature is designed to separate multiple DVB subtitle streams (like different languages on a single TV channel) into distinct output files.

### The Problem
Users rely on clean, readable subtitles. However, splitting streams introduces complexity. If the decoder isn't careful, it might leak state between streams or misinterpret timing signals, leading to garbled or repeated text. Our goal was to rigorously test this feature against a variety of inputs to ensure stability and correctness.

### What You'll Learn
In this report, we detail:
*   **The Testing Strategy:** How we designed a plan covering happy paths and edge cases.
*   **The Execution:** Step-by-step results from our verification runs.
*   **The Discovery:** A critical analysis of "Bug 1" – a massive subtitle repetition issue.
*   **The Root Cause:** A code-level deep dive into why `ccextractor` was repeating lines 1200+ times.

---

## 2. Test Plan & Methodology

We devised a 6-point test plan to stress-test the feature.

*   **Test 1: File Creation:** Does the flag actually generate separate files?
*   **Test 2: Content Verification:** Are the files valid SRTs or just empty shells?
*   **Test 3: Repetition Check:** Are we seeing the dreaded "looping subtitle" bug?
*   **Test 4: Volume Comparison:** Does the split output match the non-split output in total line count?
*   **Test 5: Edge Case (No DVB):** Does it handle files without DVB streams gracefully?
*   **Test 6: Edge Case (Single Stream):** Does it work when there's only one stream to "split"?

---

## 3. Execution & Results

### 🟢 Test 1: File Creation
**Command:** `./ccextractor multiprogram_spain.ts --split-dvb-subs -o /tmp/test_dvb`
**Result:** PASSED.
The tool correctly identified multiple streams and created individual files:
*   `test_dvb_spa_0x00D3.srt`
*   `test_dvb_spa_0x03F3.srt` (Main content)
*   ...and others.

### 🟢 Test 2: Content Verification
**Command:** `head -n 20 /tmp/test_dvb_spa_0x03F3.srt`
**Result:** PASSED.
The output file contained valid SRT formatting with timestamps and text:
```
1
00:00:00,000 --> 00:01:04,999
<font color="#f7f7f7">Laicompania'Yllana cambia</font>
```

### 🔴 Test 3: Repetition Check (The Bug)
**Command:** `uniq -c` on subtitle lines.
**Result:** **FAILED CRITICALLY.**
We found a massive repetition bug. A single subtitle line was repeated **1261 times**.
```
1261 <font color="#f7f7f7">Laicompania'Yllana cambia</font>
1261 <font color="#f7f7f7">por unos dias</font>
```

### 🔴 Test 4: Comparison Test
**Command:** `wc -l` Comparison.
**Result:** **FAILED.**
*   **Non-Split Mode:** ~80 lines of output.
*   **Split Mode:** 6305 lines of output.
The split mode is generating ~78x more lines than necessary, confirming the repetition issue is systemic.

### 🟢 Tests 5 & 6: Edge Cases
**Result:** PASSED.
*   **No DVB:** `BBC1.mp4` exited gracefully with exit code 10 (No captions found).
*   **Single Stream:** `mp_spain...C49.ts` processed correctly without crashing.

---

## 4. Root Cause Analysis

### The Investigation
We traced the issue to `src/lib_ccx/dvb_subtitle_decoder.c`. The search for `split_dvb_subs` led us to a suspect block of code intended to "Fix Bug 3".

### The Culprit
Around line 1682, we found this logic:

```c
// FIX Bug 3: In split mode, force all regions as dirty
// This ensures each DVB stream pipeline processes all available regions
if (ccx_options.split_dvb_subs)
{
    for (display = ctx->display_list; display; display = display->next)
    {
        region = get_region(ctx, display->region_id);
        if (region)
        {
            region->dirty = 1;
        }
    }
}
```

### Why It Fails
In DVB subtitling, a "region" should only be processed (decoded/written) when it is marked **dirty**—meaning new data has arrived for it.

The code above **unconditionally forces every region to be dirty** for every single packet loop when split mode is on.
1.  Packet A arrives.
2.  Decoder checks regions.
3.  **Code forces `dirty = 1`**.
4.  Decoder writes the subtitle (even if it's the same old text).
5.  Packet B arrives (maybe just a time update).
6.  **Code forces `dirty = 1` again**.
7.  Decoder writes the same subtitle again.

This infinite re-triggering explains why we see 1261 copies of the same line. The decoder is being lied to; it's being told "I have new data" constantly.

---

## 5. Proposed Solution

The fix is straightforward: **Trust the Decoder.**

The DVB decoder has built-in logic to handle `dirty` flags correctly when valid region segments are parsed. By removing the manual "force dirty" loop, we allow the natural logic to prevail:
*   Only write a subtitle when a new `DVBSUB_REGION_SEGMENT` actually updates the content.
*   This will eliminate the duplicates while retaining the correct split functionality.

---

## 6. Conclusion

The `--split-dvb-subs` feature is functional in terms of file handling but currently unusable for consumption due to massive data duplication.

**Status:**
*   **Feature:** DVB Split
*   **Verdict:** Broken (Severity: High)
*   **Next Steps:** Apply the fix to remove the forced-dirty logic in `dvb_subtitle_decoder.c` and re-run Test 4 to verify line counts match.

---

*Generated by Claude Code for the CCExtractor Project*
