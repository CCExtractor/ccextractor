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


## Compiling

### Debian/Ubuntu
    # make sure you have CCExtractor repository forked
    # clone repository
    sudo apt-get install -y git
    git clone https://github.com/%USERNAME%/ccextractor.git # paste your github username
    
    # installing dependencies
    sudo apt-get install -y gcc
    sudo apt-get install -y libcurl4-gnutls-dev
    sudo apt-get install -y tesseract-ocr
    sudo apt-get install -y tesseract-ocr-dev
    sudo apt-get install -y libleptonica-dev
    
    # compiling
    cd ccextractor/linux
    ./build
    
    # test your build
    ./ccextractor

### Fedora
    # make sure you have CCExtractor repository forked
    # clone repository
    sudo yum install -y git
    git clone https://github.com/%USERNAME%/ccextractor.git # paste your github username
    
    # installing dependencies
    sudo yum install -y gcc
    sudo yum install -y tesseract-devel # leptonica will be installed automatically
    
    # compiling
    cd ccextractor/linux
    ./build
    
    # test your build
    ./ccextractor

### Windows

Open the windows/ccextractor.sln file with Visual Studio (2015 at least), and build it. Configurations "(Debug|Release)-Full" includes dependent libraries which are used for OCR.

## Support

By far the best way to get support is by opening an issue at our [issue tracker](https://github.com/CCExtractor/ccextractor/issues). 

When you create a new issue, please fill in the needed details in the provided template. That makes it easier for us to help you more efficiently.

You can also [contact us by email or chat with the team in Slack](https://www.ccextractor.org/doku.php?id=public:general:support). 

## Contributing

You can contribute to the project by forking it, modifying the code, and making a pull request to the repository. We have some rules, outlined in the [contributor's guide](https://github.com/CCExtractor/ccextractor/blob/master/.github/CONTRIBUTING.md).

## News & Other Information

News about releases and modifications to the code can be found in the `CHANGES.TXT` file. 

For more information visit the CCExtractor website: [https://www.ccextractor.org](https://www.ccextractor.org)
