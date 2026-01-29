<img src ="https://github.com/CCExtractor/ccextractor-org-media/blob/master/static/ccx_logo_transparent_800x600.png" width="200px" alt="logo" />

# CCExtractor

[![Sample-Platform Build Status Windows](https://sampleplatform.ccextractor.org/static/img/status/build-windows.svg?maxAge=1800)](https://sampleplatform.ccextractor.org/test/master/windows)
[![Sample-Platform Build Status Linux](https://sampleplatform.ccextractor.org/static/img/status/build-linux.svg?maxAge=1800)](https://sampleplatform.ccextractor.org/test/master/linux)
[![SourceForge](https://img.shields.io/badge/SourceForge%20downloads-213k%2Ftotal-brightgreen.svg)](https://sourceforge.net/projects/ccextractor/)
[![GitHub All Releases](https://img.shields.io/github/downloads/CCExtractor/CCExtractor/total.svg)](https://github.com/CCExtractor/ccextractor/releases/latest)

CCExtractor is a tool used to produce subtitles for TV recordings from almost anywhere in the world. We intend to keep up with all sources and formats.

Subtitles are important for many people. If you're learning a new language, subtitles are a great way to learn it from movies or TV shows. If you are hard of hearing, subtitles can help you better understand what's happening on the screen. We aim to make it easy to generate subtitles by using the command line tool or Windows GUI.

The official repository is ([CCExtractor/ccextractor](https://github.com/CCExtractor/ccextractor)) and master being the most stable branch.

### **Features**

- Extract subtitles in real-time
- Translate subtitles
- Extract closed captions from DVDs
- Convert closed captions to subtitles

### Programming Languages & Technologies

The core functionality is written in C. Other languages used include C++ and Python.

## Installation and Usage

Downloads for precompiled binaries and source code can be found [on our website](https://ccextractor.org/public/general/downloads/).

### WebVTT Output Options

CCExtractor supports optional WebVTT-specific headers for advanced use cases
such as HTTP Live Streaming (HLS).

#### `--timestamp-map`

Enable writing the `X-TIMESTAMP-MAP` header in WebVTT output.

This header is required for HLS workflows but is **disabled by default**
to preserve compatibility with standard WebVTT players.

Example:
```bash
ccextractor input.ts --timestamp-map -o output.vtt


### Windows Package Managers

**WinGet:**
```powershell
winget install CCExtractor.CCExtractor
```

**Chocolatey:**
```powershell
choco install ccextractor
```

**Scoop:**
```powershell
scoop bucket add extras
scoop install ccextractor
```

Extracting subtitles is relatively simple. Just run the following command:

`ccextractor <input>`

This will extract the subtitles.

More usage information can be found on our website:

- [Using the command line tool](https://ccextractor.org/public/general/command_line_usage/)
- [Using the Flutter GUI](https://ccextractor.org/public/general/flutter_gui/)

You can also find the list of parameters and their brief description by running `ccextractor` without any arguments.

You can find sample files on [our website](https://ccextractor.org/public/general/tvsamples/) to test the software.

### Building from Source

- [Building on Windows using WSL](docs/build-wsl.md)

#### Linux (Autotools) build notes

CCExtractor also supports an autotools-based build system under the `linux/`
directory.

Important notes:
- The autotools workflow lives inside `linux/`. The `configure` script is
  generated there and should be run from that directory.
- Typical build steps are:
```
cd linux
./autogen.sh
./configure
make
```
- Rust support is enabled automatically if `cargo` and `rustc` are available
  on the system. In that case, Rust components are built and linked during
  `make`.
- If you encounter unexpected build or linking issues, a clean rebuild
  (`make clean` or a fresh clone) is recommended, especially when Rust is
  involved.

This build flow has been tested on Linux and WSL.

## Compiling CCExtractor

To learn more about how to compile and build CCExtractor for your platform check the [compilation guide](https://github.com/CCExtractor/ccextractor/blob/master/docs/COMPILATION.MD).

## Support

By far the best way to get support is by opening an issue at our [issue tracker](https://github.com/CCExtractor/ccextractor/issues).

When you create a new issue, please fill in the needed details in the provided template. That makes it easier for us to help you more efficiently.

If you have a question or a problem you can also [contact us by email or chat with the team in Slack](https://ccextractor.org/public/general/support/).

If you want to contribute to CCExtractor but can't submit some code patches or issues or video samples, you can also [donate to us](https://sourceforge.net/donate/index.php?group_id=190832)

## Contributing

You can contribute to the project by reporting issues, forking it, modifying the code and making a pull request to the repository. We have some rules, outlined in the [contributor's guide](.github/CONTRIBUTING.md).

## News & Other Information

News about releases and modifications to the code can be found in the [CHANGES.TXT](docs/CHANGES.TXT) file.

For more information visit the CCExtractor website: [https://www.ccextractor.org](https://www.ccextractor.org)

## License

GNU General Public License version 2.0 (GPL-2.0)
