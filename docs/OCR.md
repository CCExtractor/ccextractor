# Overview
OCR (Optical Character Recognition) is a technique used to 
extract text from images. In the World of Subtitle, subtitle stored 
in bitmap format are common and even necessary. For converting subtitle 
in bitmap format to subtitle in text format OCR is used.

# Dependency
1. Tesseract (OCR library by Google)
2. Leptonica (Image processing library)

# How to compile CCExtractor on Linux with OCR

## Install Dependency

### Using package manager 
#### Ubuntu, Debian
```
sudo apt-get install libleptonica-dev libtesseract-dev tesseract-ocr-eng
```
#### Suse
```
zypper install leptonica-devel
```

### Downloading source code and compiling it.

#### Leptonnica.
This package is available in your distro, you need liblept-devel library.

If Leptonica isn't available for your distribution, or you want to use a newer version
 than they offer, you can compile your own.

you can download lib leptonica source code from  http://www.leptonica.com/download.html

#### Tesseract.
Tesseract is available directly from many Linux distributions. The package is generally
 called 'tesseract' or 'tesseract-ocr' - search your distribution's repositories to
 find it. Packages are also generally available for language training data (search the
 repositories,) but if not you will need to download the appropriate training data,
 unpack it, and copy the .traineddata file into the 'tessdata' directory, probably
 /usr/share/tesseract-ocr/tessdata or /usr/share/tessdata.

If Tesseract isn't available for your distribution, or you want to use a newer version
 than they offer, you can compile your own.

If you compile Tesseract then following command in its source code are enough
```
./autogen.sh
./configure
make
sudo make install
sudo ldconfig
```

Note: 
1. CCExtractor is tested with Tesseract 3.04 version but it works with older versions. 
2. Useful Download links:
    1. *Tesseract*  https://github.com/tesseract-ocr/tesseract/archive/3.04.00.tar.gz
    2. *Tesseract training data* https://github.com/tesseract-ocr/tessdata/archive/3.04.00.tar.gz


##Compilation

###using Build script
```
cd ccextractor/linux
./build
```

### Passing flags to configure
```
cd ccextractor/linux
./autogen.sh
./configure --with-gui --enable-ocr
make
```

### Passing flags to cmake
```
cd <CCExrtactor cloned code>
mkdir build
cd build
cmake -DWITH_OCR=ON ../src
make
```



How to compile CCExtractor on Windows with OCR
===============================================

Download prebuild library of leptonica and tesseract from following link  
https://drive.google.com/file/d/0B2ou7ZfB-2nZOTRtc3hJMHBtUFk/view?usp=sharing  

put the path of libs/include of leptonica and tesseract in library paths.  
1. In visual studio 2013 right click <Project> and select property.
2. Select Configuration properties in left panel(column) of property.
3. Select VC++ Directory.
4. In the right pane, in the right-hand column of the VC++ Directory property, open the drop-down menu and choose Edit.
5. Add path of Directory where you have kept uncompressed library of leptonica and tesseract.


Set preprocessor flag ENABLE_OCR=1  
1. In visual studio 2013 right click <Project> and select property.
2. In the left panel, select Configuration Properties, C/C++, Preprocessor.
3. In the right panel, in the right-hand column of the Preprocessor Definitions property, open the drop-down menu and choose Edit.
4. In the Preprocessor Definitions dialog box, add ENABLE_OCR=1. Choose OK to save your changes.

Add library in linker
1. Open property of project
2. Select Configuration properties
3. Select Linker in left panel(column)
4. Select Input
5. Select Additional dependencies in right panel
6. Add libtesseract304d.lib in new line
7. Add liblept172.lib in new line

Download language data from following link  
https://code.google.com/p/tesseract-ocr/downloads/list  
after downloading the tesseract-ocr-3.02.eng.tar.gz extract the tar file and put  
tessdata folder where you have kept CCExtractor executable  

Copy the tesseract and leptonica dll from lib folder downloaded from above link to folder of executable or in system32.
