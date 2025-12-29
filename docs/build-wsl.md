# Building CCExtractor on Windows using WSL

This guide explains how to build CCExtractor on Windows using WSL (Ubuntu).
It is based on a fresh setup and includes all required dependencies and
common build issues encountered during compilation.

---

## Prerequisites

- Windows 10 or Windows 11
- WSL enabled
- Ubuntu installed via Microsoft Store

---

## Install WSL and Ubuntu

From PowerShell (run as Administrator):

```powershell
wsl --install -d Ubuntu
```

Restart the system if prompted, then launch Ubuntu from the Start menu.

---

## Update system packages

```bash
sudo apt update
```

---

## Install basic build tools

```bash
sudo apt install -y build-essential git pkg-config
```

---

## Install Rust (required)

CCExtractor includes Rust components, so Rust and Cargo are required.

```bash
curl https://sh.rustup.rs -sSf | sh
source ~/.cargo/env
```

Verify installation:

```bash
cargo --version
rustc --version
```

---

## Install required libraries

```bash
sudo apt install -y \
  libclang-dev clang \
  libtesseract-dev tesseract-ocr \
  libgpac-dev
```

---

## Clone the repository

```bash
git clone https://github.com/CCExtractor/ccextractor.git
cd ccextractor
```

---

## Build CCExtractor

```bash
cd linux
./build
```

After a successful build, verify by running:

```bash
./ccextractor
```

You should see the help/usage output.

---

## Common build issues

### cargo: command not found

```bash
source ~/.cargo/env
```

---

### Unable to find libclang

```bash
sudo apt install libclang-dev clang
```

---

### gpac/isomedia.h: No such file or directory

```bash
sudo apt install libgpac-dev
```

---

### please install tesseract development library

```bash
sudo apt install libtesseract-dev tesseract-ocr
```

---

## Notes

- Compiler warnings during the build process are expected and do not indicate failure.
- This guide was tested on Ubuntu (WSL) running on Windows 11.
