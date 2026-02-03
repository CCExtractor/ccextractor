# Sample Test Files

This directory contains test scripts for verifying the WebVTT X-TIMESTAMP-MAP fix.

## Test Files

The tests automatically create minimal test files (empty TS packets) to verify:
1. WebVTT header is always written
2. X-TIMESTAMP-MAP is always present
3. Fallback value is used when no timing info exists

## Using Real Video Samples

For more comprehensive testing with real video files, you can:

1. **Download a sample TS file:**
   ```bash
   curl -L "https://filesamples.com/samples/video/ts/sample_960x540.ts" -o sample.ts
   ```

2. **Run the tests:**
   ```bash
   ./run_tests.sh /path/to/ccextractor
   ```

The test script will automatically detect and use `sample.ts` if it exists.

## Expected Output

```
==========================================
WebVTT X-TIMESTAMP-MAP Test Suite
==========================================
Test: empty_ts_no_subs... PASSED [X-TIMESTAMP-MAP=MPEGTS:0,LOCAL:00:00:00.000]
Test: invalid_datapid... PASSED [X-TIMESTAMP-MAP=MPEGTS:0,LOCAL:00:00:00.000]
Test: real_sample... PASSED [X-TIMESTAMP-MAP=MPEGTS:129003,LOCAL:00:00:00.000]
==========================================
Results: 3 passed, 0 failed
==========================================
```
