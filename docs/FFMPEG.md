# Overview

FFmpeg Integration was done to support multiple encapsulations.

## Dependencies
FFmpeg libraries

### Download and Install FFmpeg on your Linux pc:
Download latest source code from following link
https://ffmpeg.org/download.html

Then following command to install ffmpeg:
`./configure && make && make install`

Note:If you installed ffmpeg on non-standard location, please change/update your
	 environment variable `$PATH` and `$LD_LIBRARY_PATH`

### Download and Install FFmpeg on your Windows pc:
1. Download vcpkg (prefer version `2023.02.24` as it is supported)
2. Integrate vcpkg into your system, run the below command in the downloaded vcpkg folder:
	```
	vcpkg integrate install
	```
3. Set Environment Variable for Vcpkg triplet, you can choose between x86 or x64 based on your system.
	```
	setx VCPKG_DEFAULT_TRIPLET "x64-windows-static-md"
	```
4. Install ffmpeg from vcpkg


	In this step we are using `x64-windows-static-md` triplet, but you will have to use the triplet you set in Step 3

	```
	vcpkg install ffmpeg --triplet x64-windows-static-md
	```

## How to compile ccextractor

### On Linux:
`make ENABLE_FFMPEG=yes`

### On Windows:
#### Set preprocessor flag `ENABLE_FFMPEG=1`
1. In visual studio 2013 right click <Project> and select property.
2. In the left panel, select Configuration Properties, C/C++, Preprocessor.
3. In the right panel, in the right-hand column of the Preprocessor Definitions property, open the drop-down menu and choose Edit.
4. In the Preprocessor Definitions dialog box, add `ENABLE_FFMPEG=1`. Choose OK to save your changes.
