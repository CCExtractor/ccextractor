# CCExtractor Docker Image

This Dockerfile builds CCExtractor with support for multiple build variants.

## Build Variants

| Variant | Description | Features |
|---------|-------------|----------|
| `minimal` | Basic CCExtractor | No OCR support |
| `ocr` | With OCR support (default) | Tesseract OCR for bitmap subtitles |
| `hardsubx` | With burned-in subtitle extraction | OCR + FFmpeg for hardcoded subtitles |

## Building

### Standalone Build (from Dockerfile only)

You can build CCExtractor using just the Dockerfile - it will clone the source from GitHub:

```bash
# Default build (OCR enabled)
docker build -t ccextractor docker/

# Minimal build (no OCR)
docker build --build-arg BUILD_TYPE=minimal -t ccextractor docker/

# HardSubX build (OCR + FFmpeg for burned-in subtitles)
docker build --build-arg BUILD_TYPE=hardsubx -t ccextractor docker/
```

### Build from Cloned Repository (faster)

If you have already cloned the repository, you can use local source for faster builds:

```bash
git clone https://github.com/CCExtractor/ccextractor.git
cd ccextractor

# Default build (OCR enabled)
docker build --build-arg USE_LOCAL_SOURCE=1 -f docker/Dockerfile -t ccextractor .

# Minimal build
docker build --build-arg USE_LOCAL_SOURCE=1 --build-arg BUILD_TYPE=minimal -f docker/Dockerfile -t ccextractor .

# HardSubX build
docker build --build-arg USE_LOCAL_SOURCE=1 --build-arg BUILD_TYPE=hardsubx -f docker/Dockerfile -t ccextractor .
```

## Build Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `BUILD_TYPE` | `ocr` | Build variant: `minimal`, `ocr`, or `hardsubx` |
| `USE_LOCAL_SOURCE` | `0` | Set to `1` to use local source instead of cloning |
| `DEBIAN_VERSION` | `bookworm-slim` | Debian version to use as base |

## Usage

### Basic Usage

```bash
# Show version
docker run --rm ccextractor --version

# Show help
docker run --rm ccextractor --help
```

### Processing Local Files

Mount your local directory to process files:

```bash
# Process a video file with output file
docker run --rm -v $(pwd):$(pwd) -w $(pwd) ccextractor input.mp4 -o output.srt

# Process using stdout
docker run --rm -v $(pwd):$(pwd) -w $(pwd) ccextractor input.mp4 --stdout > output.srt
```

### Interactive Mode

```bash
docker run --rm -it --entrypoint=/bin/bash ccextractor
```

## Image Size

The multi-stage build produces runtime images:
- `minimal`: ~130MB
- `ocr`: ~215MB (includes Tesseract)
- `hardsubx`: ~610MB (includes Tesseract + FFmpeg)
