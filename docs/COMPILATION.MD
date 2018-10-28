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
sudo apt-get install -y libglew-dev
sudo apt-get install -y libglfw3-dev
sudo apt-get install -y cmake
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

# make build systemwide
sudo make install
```

**Using CMake**

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

# make build systemwide
sudo make install
```


**Compiling with GUI:**

To build CCExtractor with a gui you will additionally need to install [GLEW](http://glew.sourceforge.net/build.html) and [GLFW](http://www.glfw.org/docs/latest/compile.html)

In order to compile it you'll need to configure it using autoconf by passing the `-with-gui` option.

```
./autogen.sh
./configure --with-gui
make

# make build systemwide
sudo make install
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

**Using CMake**

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

Place a note: Following screenshots and steps are written on basis of visualstudio version:

1.Open `windows/` directory to locate `ccextractor.vcxproj`,  `ccextractorGUI.vcxproj` (blue arrows) and `ccextractor.sln` (red arrow).

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

**Using CMake**

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

