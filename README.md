A simple utility for joining two video files that may overlap.

## Overview
The tool takes two video files and outputs a single file that contains the "unique" parts of the files.

It does this by using the timestamps of the files to find the point at which the two files overlap.

If there is no overlap between the two files, the end result is the two files will be "concatenated" together.

### Input
```
+-----------------------+
|     Video File 1      |
+-----------------------+
                  +-----------------------+
                  |     Video File 2      |
                  +-----------------------+
---------------------------------------------------> Time
```
### Output
```
+-----------------------------------------+
|            Output Video File            |
+-----------------------------------------+
```

## Limitations
This tool only supports video files with a single audio and single video track.

Any files with more tracks will have those tracks discarded e.g If a file
also contains a subtitle track, it will be absent from the output file.

## Requirements
Requires Debian >= 9.0 or Ubuntu version >= 18.04
(Has not been tested on other distributions).

This tool requires some libraries from `ffmpeg`.
All requirements can be installed on Debian/Ubuntu with:

`sudo apt-get install build-essential libavcodec-dev libavformat-dev libavutil-dev`

## Building
Running `make` will build the tool and place it at `./build/reconstruct`

## Usage
`reconstruct <FilePart1> <FilePart2> <OutputFile>`

