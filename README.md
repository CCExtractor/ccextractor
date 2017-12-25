
![logo](https://avatars3.githubusercontent.com/u/7253637?v=3&s=100)
 
# CCExtractor
What's CCExtractor?

A tool that analyzes video files and produces independent subtitle files from the closed captions data. CCExtractor is portable, small, and very fast. It works in Linux, Windows, and OSX. 
[![Build Status](https://travis-ci.org/CCExtractor/ccextractor.svg?branch=master)](https://travis-ci.org/CCExtractor/ccextractor)


More information [here](https://ccextractor.org/public:general:about_the_org). 

The official repository is ([CCExtractor/ccextractor](https://github.com/CCExtractor/ccextractor)) and master being the most stable branch. It is primarily written in C.

## Google Code-in 2017
* CCExtractor is [participating in Google Code-in 2017!](https://ccextractor.org/public:codein:welcome_2017) 

* Google Code-In is a competition of encouraging young people to learn more about Open Source and contributing to it. Tasks range from coding, documentation, quality assurance, user interface, outreach and research.

* This is our second year of challenging tasks for pre-university students aged 13-17.

* If you are a student fitting the age criteria, interested to contribute to CCExtractor feel free to join us. You can read more at the [Google Code-in website](https://codein.withgoogle.com).

* If you're interested in design tasks, you should read [this](https://www.ccextractor.org/public:codein:google_code-in_2017_code-in_for_designers) first. 

## Installation and Usage

#### CCExtractor is hosted in sourceforge. This is the download page and this is the project summary page.
Downloads for precompiled binaries and source code can be found [on our website](https://www.ccextractor.org?id=public:general:downloads).

Extracting subtitles is relatively simple. Just run the following command:

```ccextractor <input>```

This will extract the subtitles.

More usage information can be found on our website:

- [Using the command line tool](https://www.ccextractor.org/doku.php?id=public:general:command_line_usage)
- [Using the Windows GUI](https://www.ccextractor.org/doku.php?id=public:general:win_gui_usage)

You can also find the list of parameters and their brief description by running `ccextractor` without any arguments.

## How easy is it to use CCExtractor ?

It is possible to integrate `CCExtractor` in a larger process. A couple of tools already call CCExtractor as part their video process - this way they get subtitle support for free.
Starting in `0.52, CCExtractor` is very front-end friendly. Front-ends can easily get real-time status information. The GUI source code is provided and can be used for reference.
Any tool, commercial or not, is specifically allowed to use CCExtractor for any use the authors seem fit. So if your favourite video tools still lacks captioning tool, feel free to send the authors here.

## Compiling CCExtractor

To learn more about how to compile and build CCExtractor for your platform check the [compilation guide](docs/COMPILATION.MD).

## How to use subtitles once they are in a separate file?

CCExtractor generates files in the two most common formats: .srt (SubRip) and .smi (which is a Microsoft standard). Most players support at least .srt natively. You just need to name the .srt file as the file you want to play it with, for example sample.avi and sample.srt.

What kind of files can I extract closed captions from?
CCExtractor currently handles:

1. DVDs.
2. Most HDTV captures (where you save the Transport Stream).
3. Captures where captions are recorded in bttv format. The number of cards that use this card is huge. My test samples came from a Hauppage PVR-250. You can check the complete list here.
4. DVR-MS (microsoft digital video recording).
5. Tivo files 
6. ReplayTV files 
7. Dish Network files 

Usually, if you record a TV show with your capture card and CCExtractor produces the expected result, it will work for your all recordings. If it doesn't, which means that your card uses a format CCExtractor can't handle, please contact me and we'll try to make it work.

## Support

By far the best way to get support is by opening an issue at our [issue tracker](https://github.com/CCExtractor/ccextractor/issues). 

When you create a new issue, please fill in the needed details in the provided template. That makes it easier for us to help you more efficiently.

If you have a question or a problem you can also [contact us by email or chat with the team in Slack](https://www.ccextractor.org/doku.php?id=public:general:support). 

If you want to contribute to CCExtractor but can't submit some code patches or issues or video samples, you can also [donate to us](https://www.ccextractor.org/public:general:http:sourceforge.net_donate_index.php?group_id=190832) 

## Contributing

You can contribute to the project by reporting issues, forking it, modifying the code and making a pull request to the repository. We have some rules, outlined in the [contributor's guide](.github/CONTRIBUTING.md).

## News & Other Information

News about releases and modifications to the code can be found in the [CHANGES.TXT](docs/CHANGES.TXT) file. 

For more information visit the CCExtractor website: [https://www.ccextractor.org](https://www.ccextractor.org)
