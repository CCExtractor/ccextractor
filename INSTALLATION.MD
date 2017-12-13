# Installation
CCExtractor runs on all major platforms and getting started and using it is easy!

You can compile it on most platforms using CMake using the `CMakeLists.txt` file stored in `ccextractor/src` 

Autoconf and custom build scripts are also available.

 For instructions on how to install CCExtractor using them is available in the section for your platform.

There are also precompiled binaries for you too use. You can get them from [CCExtractor’s](https://www.ccextractor.org/?id=public:general:downloads) website

## Linux

### Installation

In Order to install CCExtractor you need to have the following libraries installed: `gcc,tesseract-ocr,tesseract-orc-dev,libleptonica-dev,libcurl4-gnutils-dev`

You can install them using apt-get or paste this in your terminal:

```
sudo apt-get install -y gcc
sudo apt-get install -y libcurl4-gnutls-dev
sudo apt-get install -y tesseract-ocr
sudo apt-get install -y tesseract-ocr-dev
sudo apt-get install -y libleptonica-dev
```

*Note:* On Ubuntu Version 14.04 (Trusty) and earlier, you should build leptonica and tesseract from source

### Setup
There are 3 ways to set up ccextractor depending on your choice.

#### Using the Build script

First and easiest way is to go in the linux folder and use the build script.

`cd linux && ./build`

#### Using autoconf

 Standard linux compilation through Autoconf scripts.
You'll need to have autoconf installed using apt-get.
`./autogen.sh && ./configure && make`

#### Using Cmake

Create a build folder in ccextractor and make from `src`.

`mkdir build && cd build; cmake ../src/ && make`

After Installation you can now use CCExtractor. To use it call CCExtractor from the terminal by doing `./ccextractor <file>`

### GUI:

To use CCExtractor with a gui you will additionally need to install [GLEW](http://glew.sourceforge.net/build.html) and [GLFW](http://www.glfw.org/docs/latest/compile.html)

In order to compile it you'll need to configure it using autoconf. The only difference passing the `-with-gui` option to

`./configure`.

`./autogen.sh && ./configure --with-gui && make`

Once set up you can run the GUI interface from the terminal `./ccextractorGUI`

### Fedora

### Installation

In Order to install CCExtractor you need to have the following libraries installed: `gcc,tesseract-devel` Don't worry tesseract-devel comes default with leptonica

You can install them using yum or paste this in your terminal
```
sudo yum install -y gcc
sudo yum install -y tesseract-devel
```

### Setup

There are 3 ways to set up ccextractor depending on your choice.

#### Using the Build script

First and easiest way is to go in the linux folder and use the build script.

`cd linux && ./build`

#### Using autconf

Standard linux compilation through Autoconf scripts.
You'll need to have autoconf and automake installed using yum to generate configuration script.

`./autogen.sh && ./configure && make`

#### Using Cmake

Create a build folder in ccextractor and make from `src`.

`mkdir build && cd build; cmake ../src/ && make`

After Installation you can now use CCExtractor. To use it call CCExtractor from the terminal by doing `./ccextractor <file>`

### GUI:
To use CCExtractor with a gui you will additionally need to install [GLEW](http://glew.sourceforge.net/build.html) and [GLFW](http://www.glfw.org/docs/latest/compile.html)

In order to compile it you'll need to configure it using autoconf. The only difference passing the `-with-gui` option to `./configure`.

`./autogen.sh && ./configure --with-gui && make`

Once set up you can run the GUI interface from the terminal `./ccextractorGUI`

### Mac

### Installation

In Order to install CCExtractor on your MacOS device you need to have the following libraries installed: `pkg-config, autoconf,automake, libtool, tesseract, leptonica`


You can install them using homebrew or paste this in your terminal

```
brew install pkg-config
brew install autoconf automake libtool
brew install tesseract
brew install leptonica 
```

To verify that leptonica and tesseract are succesfully installed run pkg-config to check for errors. You can test by using this command:

```
pkg-config --exists --print-errors tesseract
pkg-config --exists --print-errors lept
```

### Setup

There are 3 ways to set up ccextractor depending on your choice.

#### Using the Build script

First and easiest way is to go in the mac folder and use the build script.

`cd mac && ./build.command`

#### Using autoconf

Standard linux compilation through Autoconf scripts.

`./autogen.sh && ./configure && make`

#### Using Cmake

Create a build folder in ccextractor and make from `src`.

`mkdir build && cd build; cmake ../src/ && make`


After Installation you can now use CCExtractor. To use it call CCExtractor from the terminal by doing `./ccextractor <file>`

### GUI:

To use CCExtractor with a gui you will additionally need to install GLEW and GLFW. You can do that by installing it via homebrew using:

```
brew install glfw
brew install glew
```

In order to compile it you'll need to configure it using autoconf. The only difference passing the `-with-gui` option to `./configure`.

`./autogen.sh && ./configure --with-gui && make`

Once set up you can run the GUI interface from the terminal `./ccextractorGUI`

## Windows

### Installation

Open the windows/ccextractor.sln file with Visual Studio (2015 at least), and build it.

Configurations options are: `(Debug|Release)-Full`

Configurations options include dependent libraries which are used for OCR.


## Building Installation Packages
### Arch Linux

Go to the package_creators folder using `cd` and run the `./arch.sh`

### Redhat Package Manager (rpm) based Linux Distributions

Go to the package_creators folder using `cd` and run the `./rpm.sh`

