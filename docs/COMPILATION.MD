# Compiling CCExtractor

You may compile CCExtractor across all major platforms using `CMakeLists.txt` stored under `ccextractor/src/` directory. Autoconf and custom build scripts are also available. See platform specific instructions in the below sections.

Downloads for precompiled binaries and source code can be found [on our website](https://www.ccextractor.org?id=public:general:downloads).

Clone the latest repository from Github

```
git clone https://github.com/CCExtractor/ccextractor.git
```

## Linux

1. Make sure all the dependencies are met.

```
sudo apt-get install -y gcc
sudo apt-get install -y libcurl4-gnutls-dev
sudo apt-get install -y tesseract-ocr
sudo apt-get install -y tesseract-ocr-dev
sudo apt-get install -y libleptonica-dev
```

**Note:** On Ubuntu Version 14.04 (Trusty) and earlier, you should build leptonica and tesseract from source

2. Compiling


**Using the build script**


```
#Navigate to linux directory and call the build script

cd ccextractor/linux
./build

# test your build
./ccextractor
```

**Standard linux compilation through Autoconf scripts**

```
sudo apt-get install autoconf      #Dependency to generate configuration script
cd ccextractor/linux
./autogen.sh
./configure
make

# test your build
./ccextractor
```

**Using Cmake**

```
#Create and navigate to directory where you want to store built files

cd ccextractor/
mkdir build
cd build

#Generate makefile using cmake and then compile

cmake ../src/
make

# test your build
./ccextractor
```


**Compiling with GUI:**

To build CCExtractor with a gui you will additionally need to install [GLEW](http://glew.sourceforge.net/build.html) and [GLFW](http://www.glfw.org/docs/latest/compile.html)

In order to compile it you'll need to configure it using autoconf by passing the `-with-gui` option.

```
./autogen.sh
./configure --with-gui
make
```

Once set up you can run the GUI interface from the terminal `./ccextractorGUI`

## macOS

1. Make sure all the dependencies are met. They can be installed via Homebrew as

```
brew install pkg-config
brew install autoconf automake libtool
brew install tesseract
brew install leptonica
```

To verify tesseract and leptonica are detected by pkg-config, e.g.

````
pkg-config --exists --print-errors tesseract
pkg-config --exists --print-errors lept
````

2. Compiling

**Using build.command script:**

```
cd ccextractor/mac
./build.command OCR

# test your build
./ccextractor
```

If you don't want the OCR capabilities, then you don't need to configure the tesseract and leptonica packages, and build it with just

```
cd ccextractor/mac
./build.command

# test your build
./ccextractor
```

**Using Cmake**

```
#Create and navigate to directory where you want to store built files

cd ccextractor/
mkdir build
cd build

#Generate makefile using cmake and then compile

cmake ../src/
make

# test your build
./ccextractor
```

**Standard compilation through Autoconf scripts :**

```
cd ccextractor/mac
./autogen.sh
./configure
make

# test your build
./ccextractor
```

**Compiling with GUI:**

To use CCExtractor with a gui you will additionally need to install GLEW and GLFW. You can do that by installing it via homebrew using:

```
brew install glfw
brew install glew
```

In order to compile it you'll need to configure it using autoconf by passing the `-with-gui` option.

```
./autogen.sh
./configure --with-gui
make
```

Once set up you can run the GUI interface from the terminal `./ccextractorGUI`

## Windows

Open the windows/ccextractor.sln file with Visual Studio (2015 at least), and build it.

Configurations options are: `(Debug|Release)-Full`

Configurations options include dependent libraries which are used for OCR.

**Using Cmake**

You may also generate `.sln` files for Visual Studio and build using build tools, or open `.sln` files using Visual Studio.

```
cmake ../src/ -G "Visual Studio 14 2015"
cmake --build . --config Release --ccextractor
```

## Building Installation Packages

### Arch Linux

Go to the package_creators folder using `cd` and run the `./arch.sh`

### Redhat Package Manager (rpm) based Linux Distributions

Go to the package_creators folder using `cd` and run the `./rpm.sh`

