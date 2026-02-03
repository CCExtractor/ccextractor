# WebVTT X-TIMESTAMP-MAP Test Suite

This directory contains tests to verify that CCExtractor always includes the `X-TIMESTAMP-MAP` header in WebVTT output for HLS compliance.

## What This Tests

1. **Real video with timing info** → Uses actual MPEGTS timestamp
2. **Empty/no subtitles** → Uses fallback `MPEGTS:0,LOCAL:00:00:00.000`
3. **Invalid datapid** → Still produces header with fallback
4. **All outputs have exactly ONE header** (no duplicates)

## Running the Tests

### Linux/WSL
```bash
cd tests/webvtt_timestamp_map
chmod +x run_tests.sh
./run_tests.sh /path/to/ccextractor
```

### Windows (PowerShell)
```powershell
cd tests\webvtt_timestamp_map
.\run_tests.ps1 -CCExtractorPath "C:\path\to\ccextractor.exe"
```

## Expected Results

All generated `.vtt` files should have:
- Line 1: `WEBVTT`
- Line 2: `X-TIMESTAMP-MAP=MPEGTS:<value>,LOCAL:<time>`
- Exactly ONE `X-TIMESTAMP-MAP` header per file

## Sample Output

With timing info available:
```
WEBVTT
X-TIMESTAMP-MAP=MPEGTS:129003,LOCAL:00:00:00.000

1
00:00:01.000 --> 00:00:03.000
Hello World
```

Without timing info (fallback):
```
WEBVTT
X-TIMESTAMP-MAP=MPEGTS:0,LOCAL:00:00:00.000
```
