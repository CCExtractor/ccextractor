# CCExtractor Requirements

## Overview
CCExtractor is a tool for extracting closed captions from video files.

## Current Focus: DVB Subtitle Fixes

### Bug 3: Empty Output Files
- **Symptom**: 0-byte output files when using `--split-dvb-subs`
- **Root Cause**: First subtitle not encoded due to `write_previous` not initialized
- **Solution**: Set `write_previous = 1` during encoder initialization

### Bug 1: Subtitle Repetition  
- **Symptom**: Same subtitle text repeated for different subtitles
- **Root Cause**: OCR text not cleared between encodes
- **Solution**: Clear `prev->last_str` after each encode

## Build Requirements
- CMake 3.10+
- GCC/Clang with C11 support
- Required libraries: leptonica, tesseract (for OCR)

## Testing Requirements
- Test files with DVB subtitles
- Verify no regression in existing functionality
- Memory leak checks with valgrind
