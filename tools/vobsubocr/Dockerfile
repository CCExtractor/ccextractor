# Dockerfile for subtile-ocr - VOBSUB to SRT converter
# Uses subtile-ocr, an actively maintained fork of vobsubocr
# https://github.com/gwen-lg/subtile-ocr

FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    pkg-config \
    libleptonica-dev \
    libtesseract-dev \
    tesseract-ocr \
    tesseract-ocr-eng \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install Rust
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Install subtile-ocr from git
RUN cargo install --git https://github.com/gwen-lg/subtile-ocr

# Create working directory
WORKDIR /data

# Default command shows help
ENTRYPOINT ["subtile-ocr"]
CMD ["--help"]
