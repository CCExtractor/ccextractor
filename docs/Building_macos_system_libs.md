# Building CCExtractor on macOS using System Libraries (-system-libs)

## Overview

This document explains how to build CCExtractor on macOS using system-installed libraries instead of bundled third-party libraries.

This build mode is required for Homebrew compatibility and is enabled via the `-system-libs` flag introduced in PR #1862.

## Why is -system-libs needed?

### Background

CCExtractor was removed from Homebrew (homebrew-core) because:

- Homebrew does not allow bundling third-party libraries
- The default CCExtractor build compiles libraries from `src/thirdparty/`
- This violates Homebrew packaging policies

### What -system-libs fixes

The `-system-libs` flag allows CCExtractor to:

- Use system-installed libraries via Homebrew
- Resolve headers and linker flags using `pkg-config`
- Skip compiling bundled copies of common libraries

This makes CCExtractor acceptable for Homebrew packaging.

## Build Modes Explained

### 1️⃣ Default Build (Bundled Libraries)

**Command:**

```bash
./mac/build.command
```

**Behavior:**

- Compiles bundled libraries:
  - `freetype`
  - `libpng`
  - `zlib`
  - `utf8proc`
- Self-contained binary
- Larger size
- Suitable for standalone builds

### 2️⃣ System Libraries Build (Homebrew-compatible)

**Command:**

```bash
./mac/build.command -system-libs
```

**Behavior:**

- Uses system libraries via `pkg-config`
- Does not compile bundled libraries
- Smaller binary
- Faster build
- Required for Homebrew

## Required Homebrew Dependencies

Install required dependencies:

```bash
brew install pkg-config autoconf automake libtool \
  gpac freetype libpng protobuf-c utf8proc zlib
```

**Optional** (OCR / HARDSUBX support):

```bash
brew install tesseract leptonica ffmpeg
```

## How to Build

```bash
cd mac
./build.command -system-libs
```

**Verify:**

```bash
./ccextractor --version
```

## What Changes Internally with -system-libs

### Libraries NOT compiled (system-provided)

- **FreeType**
- **libpng**
- **zlib**
- **utf8proc**

### Libraries STILL bundled

- **lib_hash** (Custom SHA-256 implementation, no system equivalent)

## CI Coverage

A new CI job was added:

- `build_shell_system_libs`

**What it does:**

- Installs Homebrew dependencies
- Runs `./build.command -system-libs`
- Verifies the binary runs correctly

This ensures Homebrew-compatible builds stay working.

## Verification (Local)

You can confirm system libraries are used:

```bash
otool -L mac/ccextractor
```

**Expected output includes paths like:**

```
/opt/homebrew/opt/gpac/lib/libgpac.dylib
```

## Homebrew Formula Usage (Future)

Example formula snippet:

```ruby
def install
  system "./mac/build.command", "-system-libs"
  bin.install "mac/ccextractor"
end
```

## Summary

- `-system-libs` is opt-in
- Default build remains unchanged
- Enables CCExtractor to return to Homebrew
- Fully tested in CI and locally

## Related

- **PR #1862** — Add `-system-libs` flag
- **Issue #1580** — Homebrew compatibility
- **Issue #1534** — System library support
