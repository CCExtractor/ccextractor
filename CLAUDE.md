# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CCExtractor is a tool for extracting subtitles/closed captions from video files. Written primarily in C with a growing Rust codebase, it supports multiple subtitle formats (CEA-608, CEA-708, DVB, Teletext, ISDB) and container formats (TS, PS, MP4, MKV, ASF, WTV, GXF, MXF).

## Build Commands

### Linux
```bash
cd linux/
./build                    # Basic build
./build -debug             # Build with debug symbols
./build -hardsubx          # Build with burned-in subtitle extraction (OCR)
./build -system-libs       # Build with system libraries instead of bundled
```

### CMake (Cross-platform)
```bash
mkdir build && cd build
cmake ../src/                          # Basic build
cmake ../src/ -DWITH_OCR=ON            # Enable OCR (tesseract)
cmake ../src/ -DWITH_HARDSUBX=ON       # Enable burned-in subtitles
make
```

### macOS
```bash
cd mac/
./build.command              # Basic build
./build.command -ocr         # Build with OCR
./build.command -hardsubx    # Build with hardsubx (FFmpeg 8 default on macOS)
```

### Windows
```bash
msbuild windows/ccextractor.sln /p:Configuration=Release /p:Platform=x64
```
Configurations: `Release`, `Debug`, `Release-Full` (with OCR), `Debug-Full` (with OCR)

### Dependencies

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get install -y libgpac-dev libglew-dev libglfw3-dev cmake gcc \
  libcurl4-gnutls-dev tesseract-ocr libtesseract-dev libleptonica-dev clang libclang-dev
```

**Rust:** Version 1.87.0 or above required.

## Testing

### C Unit Tests
```bash
cd tests/
make           # Build and run all tests
DEBUG=1 make   # Run with verbose output
```
Tests use libcheck framework. The `tests/runtest.c` runner always executes the full suite.

### Rust Tests
```bash
cd src/rust/
cargo test                       # Test main Rust module

cd src/rust/lib_ccxr/
cargo test                       # Test lib_ccxr module
cargo test <test_name_pattern>   # Run specific tests
```

### Integration Testing
The project uses a [Sample Platform](https://sampleplatform.ccextractor.org/) for automated testing against real video samples on commits to master.

## Running CCExtractor

```bash
./ccextractor input_video.ts    # Extract subtitles
./ccextractor                   # Show full help
```

## Architecture Overview

### High-Level Flow
```
Input Files → Demuxer → Decoder → Encoder → Output Files
```

1. **Input Layer** (`ccextractor.c`, `lib_ccx/file_buffer.c`): File I/O, stdin, network streams
2. **Demuxer Layer** (`lib_ccx/ccx_demuxer.c`, `lib_ccx/general_loop.c`): Container format detection/parsing
3. **Decoder Layer** (`lib_ccx/ccx_decoders_*.c`): Caption data extraction
4. **Encoder Layer** (`lib_ccx/ccx_encoders_*.c`): Output format conversion
5. **Output Layer**: Subtitle file writing

### Key Data Structures
- `lib_ccx_ctx` (`lib_ccx.h:112`) - Main context holding all state
- `ccx_demuxer` (`ccx_demuxer.h`) - Demuxer context with stream/PID tracking
- `demuxer_data` - Parsed caption payload passed between components
- `cc_subtitle` - Unified subtitle representation for all formats
- `ccx_s_options` (`ccx_common_option.h`) - All user-configurable parameters

### Core Components

**Demuxing (`lib_ccx/`):**
- `ccx_demuxer.c` - Main demuxer interface with auto-detection
- `ts_functions.c` - MPEG Transport Stream parsing
- `asf_functions.c` - ASF/WMV container
- `matroska.c` - MKV/WebM container
- `mp4.c` - MP4/M4V container
- `wtv_functions.c` - Windows TV recordings

**Decoding:**
- `ccx_decoders_608.c` - CEA-608 (Line 21) closed captions
- `ccx_decoders_708.c` - CEA-708 (DTVCC) digital TV captions
- `dvb_subtitle_decoder.c` - DVB subtitles
- `telxcc.c` - Teletext subtitles
- `ccx_decoders_isdb.c` - ISDB captions (Japanese)

**Encoding:**
- `ccx_encoders_srt.c` - SubRip (.srt)
- `ccx_encoders_webvtt.c` - WebVTT
- `ccx_encoders_sami.c` - SAMI
- `ccx_encoders_ssa.c` - SSA/ASS
- `ccx_encoders_transcript.c` - Plain text

**Special Features:**
- `hardsubx.c` - Burned-in subtitle extraction using OCR (Tesseract)

### Rust Components

Rust code in `src/rust/` is gradually replacing C implementations:

- `src/rust/lib_ccxr/` - Core Rust library (types, utilities)
- `src/rust/src/demuxer/` - DVD raw, SCC format parsing
- `src/rust/src/decoder/` - CEA-708/DTVCC in Rust
- `src/rust/src/libccxr_exports/` - FFI exports callable from C

**C-to-Rust interop:** C calls Rust via `ccxr_*` prefixed functions (e.g., `ccxr_process_dvdraw`, `ccxr_parse_parameters`). Build uses Corrosion to integrate Cargo into CMake.

### Timing and Synchronization
- `ccx_common_timing.c/h` - PTS/DTS timestamp handling
- Critical for subtitle timing accuracy across formats
- Handles discontinuities, wraparounds, clock references

## Development Practices

### Code Organization
- Demuxer logic in `lib_ccx/` with `<format>_functions.c` naming
- Decoders: `ccx_decoders_<standard>.c`
- Encoders: `ccx_encoders_<format>.c`
- Rust FFI exports in `src/rust/src/libccxr_exports/`

### FFmpeg Version Handling
Platform defaults: Linux/Windows = FFmpeg 6.x, macOS = FFmpeg 8.x
Override with `FFMPEG_VERSION` environment variable.

### Memory Management
- C uses manual memory management; check `ccx_demuxer_close()` for cleanup patterns
- Rust uses RAII but must carefully manage memory at FFI boundary

## Linting and Formatting

### C
```bash
find src/ -type f -not -path "src/thirdparty/*" -not -path "src/lib_ccx/zvbi/*" -name '*.c' -not -path "src/GUI/icon_data.c" | xargs clang-format -i
```

### Rust
```bash
cd src/rust && cargo fmt && cargo clippy
cd src/rust/lib_ccxr && cargo fmt && cargo clippy
```

## Common Workflows

**Adding a new container format:**
1. Add detection in `ccx_demuxer.c` → `detect_stream_type()`
2. Create `<format>_functions.c` in `lib_ccx/`
3. Add stream mode enum to `ccx_common_constants.h`
4. Integrate into `general_loop.c`
5. Update build scripts

**Adding a new caption format:**
1. Create decoder in `lib_ccx/ccx_decoders_<format>.c`
2. Add codec enum to `ccx_common_constants.h`
3. Wire into demuxer's data path
4. Add encoder if new output format needed

## Troubleshooting

- **GPAC not found (Ubuntu 23.10+):** Build from source
- **Rust version mismatch:** `rustup update stable`
- **FFmpeg issues:** Set `FFMPEG_VERSION` env var
