# VOBSUB Subtitle Extraction from MKV Files

CCExtractor supports extracting VOBSUB (S_VOBSUB) subtitles from Matroska (MKV) containers. VOBSUB is an image-based subtitle format originally from DVD video.

## Overview

VOBSUB subtitles consist of two files:
- `.idx` - Index file containing metadata, palette, and timestamp/position entries
- `.sub` - Binary file containing the actual subtitle bitmap data in MPEG Program Stream format

## Basic Usage

```bash
ccextractor movie.mkv
```

This will extract all VOBSUB tracks and create paired `.idx` and `.sub` files:
- `movie_eng.idx` + `movie_eng.sub` (first English track)
- `movie_eng_1.idx` + `movie_eng_1.sub` (second English track, if present)
- etc.

## Converting VOBSUB to SRT (Text)

Since VOBSUB subtitles are images, you need OCR (Optical Character Recognition) to convert them to text-based formats like SRT.

### Using subtile-ocr (Recommended)

[subtile-ocr](https://github.com/gwen-lg/subtile-ocr) is an actively maintained Rust tool that provides accurate OCR conversion.

#### Option 1: Docker (Easiest)

We provide a Dockerfile that builds subtile-ocr with all dependencies:

```bash
# Build the Docker image (one-time)
cd tools/vobsubocr
docker build -t subtile-ocr .

# Extract VOBSUB from MKV
ccextractor movie.mkv

# Convert to SRT using OCR
docker run --rm -v $(pwd):/data subtile-ocr -l eng -o /data/movie_eng.srt /data/movie_eng.idx
```

#### Option 2: Install subtile-ocr Natively

If you have Rust and Tesseract development libraries installed:

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libleptonica-dev libtesseract-dev tesseract-ocr tesseract-ocr-eng

# Install subtile-ocr
cargo install --git https://github.com/gwen-lg/subtile-ocr

# Convert
subtile-ocr -l eng -o movie_eng.srt movie_eng.idx
```

### subtile-ocr Options

| Option | Description |
|--------|-------------|
| `-l, --lang <LANG>` | Tesseract language code (required). Examples: `eng`, `fra`, `deu`, `chi_sim` |
| `-o, --output <FILE>` | Output SRT file (stdout if not specified) |
| `-t, --threshold <0.0-1.0>` | Binarization threshold (default: 0.6) |
| `-d, --dpi <DPI>` | Image DPI for OCR (default: 150) |
| `--dump` | Save processed subtitle images as PNG files |

### Language Codes

Install additional Tesseract language packs as needed:

```bash
# Examples
sudo apt-get install tesseract-ocr-fra  # French
sudo apt-get install tesseract-ocr-deu  # German
sudo apt-get install tesseract-ocr-spa  # Spanish
sudo apt-get install tesseract-ocr-chi-sim  # Simplified Chinese
```

## Technical Details

### .idx File Format

The index file contains:
1. Header with metadata (size, palette, alignment settings)
2. Language identifier line
3. Timestamp entries with file positions

Example:
```
# VobSub index file, v7 (do not modify this line!)
size: 720x576
palette: 000000, 828282, ...

id: eng, index: 0
timestamp: 00:01:12:920, filepos: 000000000
timestamp: 00:01:18:640, filepos: 000000800
...
```

### .sub File Format

The binary file contains MPEG Program Stream packets:
- Each subtitle is wrapped in a PS Pack header (14 bytes) + PES header (15 bytes)
- Subtitles are aligned to 2048-byte boundaries
- Contains raw SPU (SubPicture Unit) bitmap data

## Troubleshooting

### Empty output files
- Ensure the MKV file actually contains VOBSUB tracks (check with `mediainfo` or `ffprobe`)
- CCExtractor will report "No VOBSUB subtitles to write" if the track is empty

### OCR quality issues
- Try adjusting the `-t` threshold parameter
- Ensure the correct language pack is installed
- Use `--dump` to inspect the processed images

### Docker permission issues
- The output files may be owned by root; use `sudo chown` to fix ownership
- Or run Docker with `--user $(id -u):$(id -g)`

## See Also

- [OCR.md](OCR.md) - General OCR support in CCExtractor
- [subtile-ocr GitHub](https://github.com/gwen-lg/subtile-ocr) - OCR tool documentation
