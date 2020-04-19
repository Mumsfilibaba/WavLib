# WavLib

A simple loader of .wav files that only consists of two files.

## Features
* Load .wav files

## Supported formats
* Signed 8-, 16- and 32-bit integer PCM files are supported for now. It is assumed that the files are saved in litte endian format

## How to build

1. Clone repository

2. Place WavLib.h and WavLib.c in your project and compile, no other linking required

3. Can also be included in a premake build
```
include "WavLib"
```

