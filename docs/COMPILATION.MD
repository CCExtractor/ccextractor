# Compiling CCExtractor

You may compile CCExtractor across all major platforms using `CMakeLists.txt` stored under `ccextractor/src/` directory. Autoconf and custom build scripts are also available. See platform specific instructions in the below sections.

Downloads for precompiled binaries and source code can be found [on our website](https://www.ccextractor.org?id=public:general:downloads).

Clone the latest repository from Github

```bash
git clone https://github.com/CCExtractor/ccextractor.git
```

### Hardsubx (Burned-in Subtitles) and FFmpeg Versions

CCExtractor's hardsubx feature extracts burned-in subtitles from videos using OCR. It requires FFmpeg libraries. The build system automatically selects appropriate FFmpeg versions for each platform:

- **Linux**: FFmpeg 6.x (default)
- **Windows**: FFmpeg 6.x (default)
- **macOS**: FFmpeg 8.x (default)

You can override the default by setting the `FFMPEG_VERSION` environment variable to `ffmpeg6`, `ffmpeg7`, or `ffmpeg8` before building. This flexibility ensures compatibility with different FFmpeg installations across platforms.

## Docker
You can now use docker image to build latest source of CCExtractor without any environmental hustle. Follow these [instructions](https://github.com/CCExtractor/ccextractor/tree/master/docker/README.md) for building docker image & usage of it.

## Linux

1. Make sure all the dependencies are met.

Debian:

```bash
sudo apt-get install -y libgpac-dev libglew-dev libglfw3-dev cmake gcc libcurl4-gnutls-dev tesseract-ocr libtesseract-dev libleptonica-dev clang libclang-dev
```

RHEL/Fedora:

```bash
yum install -y glew-devel glfw-devel cmake gcc libcurl-devel tesseract-devel leptonica-devel clang gpac-devel
```

Arch:
```bash
sudo paru -S glew glfw curl tesseract leptonica cmake gcc clang gpac
```
or
```bash
sudo pacman -S glew glfw curl tesseract leptonica cmake gcc clang gpac
```

Rust 1.54 or above is also required. [Install Rust](https://www.rust-lang.org/tools/install). Check specific compilation methods below, on how to compile without rust.

**Note:** On Ubuntu Version 23.10 (Mantic) and later, `libgpac-dev` isn't available, you should build gpac from source by following the easy build instructions [here](https://github.com/gpac/gpac/wiki/GPAC-Build-Guide-for-Linux)

**Note:** On Ubuntu Version 18.04 (Bionic) and later, `libtesseract-dev` is installed rather than `tesseract-ocr-dev`, which does not exist anymore.

**Note:** On Ubuntu Version 14.04 (Trusty) and earlier, you should build leptonica and tesseract from source

2. Compiling

### Using the build script

By default build script does not include debugging information hence, you cannot debug the executable produced (i.e. `./ccextractor`) on a debugger. To include debugging information, use the `builddebug` script.

```bash
# navigate to linux directory and call the build script

cd ccextractor/linux

# compile without debug flags
./build

# compile with debug info
./build -debug            # same as ./builddebug

# compile with hardsubx (burned-in subtitle extraction)
# Hardsubx requires FFmpeg libraries. Different FFmpeg versions are used by default:
#   - Linux: FFmpeg 6.x (automatic)
#   - Windows: FFmpeg 6.x (automatic)
#   - macOS: FFmpeg 8.x (automatic)

./build -hardsubx         # uses platform-specific FFmpeg version

# To override the default FFmpeg version, set FFMPEG_VERSION:
FFMPEG_VERSION=ffmpeg8 ./build -hardsubx  # force FFmpeg 8 on any platform
FFMPEG_VERSION=ffmpeg6 ./build -hardsubx  # force FFmpeg 6 on any platform
FFMPEG_VERSION=ffmpeg7 ./build -hardsubx  # force FFmpeg 7 on any platform

# [Optional] For custom FFmpeg installations, set these environment variables:
FFMPEG_INCLUDE_DIR=/usr/include 
FFMPEG_PKG_CONFIG_PATH=/usr/lib/pkgconfig


# test your build
./ccextractor
```

### Standard linux compilation through Autoconf scripts

```bash
sudo apt-get install autoconf  # dependency to generate configuration script
cd ccextractor/linux
./autogen.sh
./configure
make

# test your build
./ccextractor

# make build systemwide
sudo make install
```

### Using CMake

```bash
# create and navigate to directory where you want to store built files
cd ccextractor/
mkdir build
cd build

# generate makefile using cmake and then compile
cmake ../src/  # options here
make

# test your build
./ccextractor

# make build systemwide
sudo make install
```

`cmake` also accepts the options:
     `-DWITH_OCR=ON` to enable OCR
     `-DWITH_HARDSUBX=ON` to enable burned-in subtitles (requires FFmpeg)
     
For hardsubx with specific FFmpeg versions:
     Set `FFMPEG_VERSION=ffmpeg6` for FFmpeg 6.x (default on Linux and Windows)
     Set `FFMPEG_VERSION=ffmpeg7` for FFmpeg 7.x  
     Set `FFMPEG_VERSION=ffmpeg8` for FFmpeg 8.x
     (Defaults: Linux=FFmpeg 6, Windows=FFmpeg 6, macOS=FFmpeg 8)

([OPTIONAL] For custom FFmpeg installations, set these environment variables)

     FFMPEG_INCLUDE_DIR=/usr/include 
     FFMPEG_PKG_CONFIG_PATH=/usr/lib/pkgconfig

### Compiling with GUI

The GUI for CCExtractor has been moved to a separate repository ([https://github.com/CCExtractor/ccextractorfluttergui](https://github.com/CCExtractor/ccextractorfluttergui)).

## macOS

1. Make sure all the dependencies are met. Decide if you want OCR; if so, you'll need to install tesseract and leptonica.
Dependencies can be installed via Homebrew as:

```bash
brew install pkg-config
brew install autoconf automake libtool
brew install cmake gpac
# optional if you want OCR:
brew install tesseract
brew install leptonica
# optional if you want hardsubx (burned-in subtitle extraction):
brew install ffmpeg
```

If configuring OCR, use pkg-config to verify tesseract and leptonica dependencies, e.g.

```bash
pkg-config --exists --print-errors tesseract
pkg-config --exists --print-errors lept
```

### Compiling

#### Using build.command script:

```bash
cd ccextractor/mac
./build.command              # basic build
./build.command -ocr         # build with OCR support
./build.command -hardsubx    # build with hardsubx (uses FFmpeg 8 by default on macOS)

# Override FFmpeg version if needed:
FFMPEG_VERSION=ffmpeg7 ./build.command -hardsubx

# test your build
./ccextractor
```

#### Using CMake

```bash
# create and navigate to directory where you want to store built files
cd ccextractor/
mkdir build
cd build

# generate makefile using cmake and then compile
cmake ../src/  # options here
make

# test your build
./ccextractor
```

`cmake` also accepts the options:
     `-DWITH_OCR=ON` to enable OCR
     `-DWITH_HARDSUBX=ON` to enable burned-in subtitles

#### Standard compilation through Autoconf scripts:

```bash
cd ccextractor/mac
./autogen.sh
./configure         
make

# test your build
./ccextractor
```

#### Compiling with GUI:

The GUI for CCExtractor has been moved to a separate repository ([https://github.com/CCExtractor/ccextractorfluttergui](https://github.com/CCExtractor/ccextractorfluttergui)).

## Windows
Dependencies are clang and rust. To enable OCR, rust x86_64-pc-windows-msvc or i686-pc-windows-msvc target should be installed

GPAC is also required, you can install it through chocolatey:
```
choco install gpac
```

Other dependencies are required through vcpkg, so you can follow below steps:
1. Download vcpkg (prefer version `2023.02.24` as it is supported)
2. Integrate vcpkg into your system, run the below command in the downloaded vcpkg folder:
     ```
     vcpkg integrate install
     ```
3. Set Environment Variable for Vcpkg triplet, you can choose between x86 or x64 based on your system.
     ```
     setx VCPKG_DEFAULT_TRIPLET "x64-windows-static"
     setx RUSTFLAGS "-Ctarget-feature=+crt-static"
     ```
4. Install dependencies from vcpkg

     In this step we are using `x64-windows-static` triplet, but you will have to use the triplet you set in Step 3

     if building Debug-Full, Release-Full (HardSubx)
     ```
     vcpkg install ffmpeg leptonica tesseract --triplet x64-windows-static
     ```
     Note: Windows builds use FFmpeg 6 by default. To override:
     ```
     set FFMPEG_VERSION=ffmpeg8
     msbuild ccextractor.sln /p:Configuration=Debug-Full /p:Platform=x64
     ```
     
     otherwise if you have Debug, Release
     ```
     vcpkg install libpng --triplet x64-windows-static
     ```

Note: Following screenshots and steps are based on Visual Studio 2017, but they should be more or less same for other versions.

1.Open `windows/` directory to locate `ccextractor.vcxproj` and `ccextractor.sln` (red arrow).

![Project Files](img/projectFiles.png)

2.Accept the security prompt (if any), to proceed with compilation.
![A warning you can receive](img/Warning.png)

3.Using Visual Studio (2015 or above), open ccextractor.sln. This will build both CCExtractor and its GUI. To build them separately, open the respective .vcxproj file.

4.In Solution Explorer, you'll see two projects with the VS version and Windows release version in parenthesis.  Change them to parameters which are true for you by clicking right mouse button on project and selecting properties.

![Project Section](img/ProjectSection.png)

![Properties, that you have to change](img/Properties.png)

5.Right click and select `build` to compile the project and generate executable file.

![Building button](img/Building.png)

6.Find the executable file in `Debug` or `Release` folder, based on selected configuration.

![Path to Binaries](img/Binaries.png)

Configurations options are: `(Debug|Release)-Full`

Configurations options include dependent libraries which are used for OCR.

### Using CMake

You may also generate `.sln` files for Visual Studio and build using build tools, or open `.sln` files using Visual Studio.

```bash
cmake ../src/ -G "Visual Studio 14 2015"
cmake --build . --config Release --ccextractor
```

### Using MSBuild

Run the following command in `windows/` directory

```bash
msbuild ccextractor.sln /p:Configuration=Release /p:Platform=x64
```
Different configuration options are,

| Configuration | Platform | Rust target required |
| ------------- |:-------------:| -----:|
| Release | x64 | default |
| Debug | x64 | default |
| Release-Full(OCR) | Win32 | i686-pc-windows-msvc |
| Debug-Full(OCR) | Win32 | i686-pc-windows-msvc |

## Building Installation Packages

### Arch Linux

Go to the package_creators folder using `cd` and run the `./arch.sh`

### Redhat Package Manager (rpm) based Linux Distributions

Go to the package_creators folder using `cd` and run the `./rpm.sh`
