
![logo](https://avatars3.githubusercontent.com/u/7253637?v=3&s=100)
 
# CCExtractor

CCExtractor is a tool that produces subtitles from TV use. Global accessibility (all users, all content, all countries) is the goal. With so many different formats, this is a constantly moving target, but we intend to keep up with all sources and formats.

Carlos' version (mainstream) is the most stable branch.

## Google Summer of Code 2017
CCExtractor has been invited to Summer of Code 2017! Another summer of coding fun.

If you are a student currently enrolled in university most likely you are eligible to participate. Read more at:  
- [Google Summer of Code official website ](https://summerofcode.withgoogle.com/)
- [CCExtractor's main GSoC page](https://www.ccextractor.org?id=public:gsoc:ideas_page_for_summer_of_code_2017)


## Installation and Usage

Downloads for precompiled binaries and source code can be found [on our website](https://www.ccextractor.org?id=public:general:downloads).

Extracting subtitles is relatively simple. Just run the following command:

```ccextractor <input>```

This will extract the subtitles. 

More usage information can be found on our website:

- [Using the command line tool](https://www.ccextractor.org/doku.php?id=public:general:command_line_usage)
- [Using the Windows GUI](https://www.ccextractor.org/doku.php?id=public:general:win_gui_usage) 

You can also find the list of parameters and their brief description by running `ccextractor` without any arguments.

## Compiling CCExtractor (without GUI)

You may compile CCExtractor across all major platforms using `CMakeLists.txt` stored under `ccextractor/src/` directory. Simply,

1. Create and navigate to directory where you want to store built files

```
cd ccextractor/
mkdir build
cd build
```

2. Generate makefile using cmake and then compile

```
cmake ../src/
make
```

You may also generate `.sln` files for Visual Studio and build using build tools, or open `.sln` files using Visual Studio.

```
cmake ../src/ -G "Visual Studio 14 2015"
cmake --build . --config Release --ccextractor
```

---

CCExtractor can also be compiled without cmake. System specific instructions are listed below :

Clone the latest repository from Github

```
git clone https://github.com/CCExtractor/ccextractor.git
```

### Debian/Ubuntu
   
1. Make sure all the dependencies are met.

```
sudo apt-get install -y gcc
sudo apt-get install -y libcurl4-gnutls-dev
sudo apt-get install -y tesseract-ocr
sudo apt-get install -y tesseract-ocr-dev
sudo apt-get install -y libleptonica-dev

# Note: On Ubuntu Version 14.04 (Trusty) and earlier, you should build leptonica and tesseract from source 
```
    
2. Compiling

*Using build script :*
    
```
#Navigate to linux directory and call the build script

cd ccextractor/linux
./build

# test your build
./ccextractor
```
   
*Standard linux compilation through Autoconf scripts :*

```
sudo apt-get install autoconf      #Dependency to generate configuration script
cd ccextractor/linux
./autogen.sh
./configure
make

# test your build
./ccextractor
```

### Fedora

1. Make sure all the dependencies are met.

```
sudo yum install -y gcc
sudo yum install -y tesseract-devel # leptonica will be installed automatically
```
    
2. Compiling

*Using build script :*

```    
#Navigate to linux directory and call the build script

cd ccextractor/linux
./build

# test your build
./ccextractor
```
    
*Standard linux compilation through Autoconf scripts :*

```
sudo dnf install autoconf automake      #Dependency to generate configuration script
cd ccextractor/linux
./autogen.sh
./configure
make

# test your build
./ccextractor
```

### MacOS

1. Make sure all the dependencies are met. They can be installed via Homebrew as

```
brew install pkg-config
brew install autoconf automake libtool
brew install tesseract
brew install leptonica 
```

Make sure tesseract and leptonica are detected by pkg-config, e.g.

````
pkg-config --exists --print-errors tesseract
pkg-config --exists --print-errors lept
````

2. Compiling

*Using build.command script :*

```
cd ccextractor/mac
./build.command OCR
```
If you don't want the OCR capabilities, then you don't need to configure the tesseract and leptonica packages, and build it with just
```
cd ccextractor/mac
./build.command 
```

*Standard compilation through Autoconf scripts :*

```
cd ccextractor/mac
./autogen.sh
./configure
make
```


### Windows

Open the windows/ccextractor.sln file with Visual Studio (2015 at least), and build it. Configurations "(Debug|Release)-Full" includes dependent libraries which are used for OCR.

## Compiling CCExtractor (with GUI)

### Linux
   
1. Make sure all the dependencies are met.</br>
 * Build GLEW from source, instructions [here](http://glew.sourceforge.net/build.html)
 * Build GLFW from source, instructions [here](http://www.glfw.org/docs/latest/compile.html)
    
2. Compiling

    
```
cd ccextractor/linux
./autogen.sh
./configure --with-gui
make

# test your build
./ccextractorGUI
```


### MacOS

1. Make sure all the dependencies are met. They can be installed via Homebrew as

```
brew install glfw
brew install glew
```

2. Compiling

```
cd ccextractor/mac
./autogen.sh
./configure --with-gui
make
```


### Windows

Open the windows/ccextractor.sln file with Visual Studio (2015 at least), and build it. Configurations "(Debug|Release)-Full" includes dependent libraries which are used for OCR.

## Building Installation Packages 

### Arch Linux

*building installation package (.pkg.tar.xz) or installing directly*

    cd ccextractor/package_creators
    ./arch.sh
    
### Redhat Package Manager (rpm) based Linux Distributions

*building installation package (.rpm)*

    cd ccextractor/package_creators
    ./rpm.sh

## Support

By far the best way to get support is by opening an issue at our [issue tracker](https://github.com/CCExtractor/ccextractor/issues). 

When you create a new issue, please fill in the needed details in the provided template. That makes it easier for us to help you more efficiently.

You can also [contact us by email or chat with the team in Slack](https://www.ccextractor.org/doku.php?id=public:general:support). 

## Contributing

You can contribute to the project by forking it, modifying the code, and making a pull request to the repository. We have some rules, outlined in the [contributor's guide](https://github.com/CCExtractor/ccextractor/blob/master/.github/CONTRIBUTING.md).

## News & Other Information

News about releases and modifications to the code can be found in the `CHANGES.TXT` file. 

For more information visit the CCExtractor website: [https://www.ccextractor.org](https://www.ccextractor.org)
