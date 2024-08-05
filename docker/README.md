# CCExtractor Docker image

This dockerfile prepares a minimalist Docker image with CCExtractor. It compiles CCExtractor from sources following instructions from the [Compilation Guide](https://github.com/CCExtractor/ccextractor/blob/master/docs/COMPILATION.MD).

You can install the latest build of this image by running `docker pull CCExtractor/ccextractor`

## Build

You can build the Docker image directly from the Dockerfile provided in [docker](https://github.com/CCExtractor/ccextractor/tree/master/docker) directory of CCExtractor source

```bash
$ git clone https://github.com/CCExtractor/ccextractor.git && cd ccextractor
$ cd docker/
$ docker build -t ccextractor .
```

## Usage

The CCExtractor Docker image can be used in several ways, depending on your needs.

```bash
# General usage
$ docker run ccextractor:latest <features>
```

1. Process a local file & use `-o` flag

To process a local video file, mount a directory containing the input file inside the container:

```bash
# Use `-o` to specifying output file
$ docker run --rm -v $(pwd):$(pwd) -w $(pwd) ccextractor:latest input.mp4 -o output.srt

# Alternatively use `--stdout` feature
$ docker run --rm -v $(pwd):$(pwd) -w $(pwd) ccextractor:latest input.mp4 --stdout > output.srt
```

Run this command from where your input video file is present, and change `input.mp4` & `output.srt` with the actual name of files.

2. Enter an interactive environment

If you need to run CCExtractor with additional options or perform other tasks within the container, you can enter an interactive environment:
bash

```bash
$ docker run --rm -it --entrypoint='sh' ccextractor:latest
```

This will start a Bash shell inside the container, allowing you to run CCExtractor commands manually or perform other operations.

### Example

I run help command in image built from `dockerfile`

```bash
$ docker build -t ccextractor .
$ docker run --rm ccextractor:latest --help
```

This will show the `--help` message of CCExtractor tool
From there you can see all the features and flags which can be used.
