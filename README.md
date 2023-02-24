# WoWs depack

[![Ubuntu-x86_64](https://github.com/kakwa/wows-depack/actions/workflows/ubuntu-x86_64.yml/badge.svg)](https://github.com/kakwa/wows-depack/actions/workflows/ubuntu-x86_64.yml)
[![Coverage Status](https://coveralls.io/repos/github/kakwa/wows-depack/badge.svg?branch=main)](https://coveralls.io/github/kakwa/wows-depack?branch=main)

World of Warships resource files unpacker

# Format specification

The format specification is presented in [FORMAT.md](https://github.com/kakwa/wows-depack/blob/main/FORMAT.md).

The reverse engineering story is detailed in [REVERSING_STORY.md](https://github.com/kakwa/wows-depack/blob/main/REVERSING_STORY.md)

# Usage

## CLI tool

TODO
```shell
./wows-depack-cli -i ~/Games/World\ of\ Warships/bin/6775398/idx/system_data.idx
```

## Library


Basic usage example:

```C
#include "wows-depack.h"

char *index_file_path = "path/to/file.idx"

WOWS_CONTEXT *context = wows_init_context(NO_DEBUG);

/* Other debug categories */
// DEBUG_RAW_RECORD
// DEBUG_FILE_LISTING

/* several debug categories can be enabled like so: */
// DEBUG_FILE_LISTING | DEBUG_RAW_RECORD;

/* Parse the index file */
int ret = wows_parse_index(index_file_path, context);

/* Error handling */
if (ret != 0) {
    /* get an error message + additional info from context + return code */
    char *err_msg = wows_error_string(ret, context);
    printf("Error: %s\n", err_msg);

    // the message must be freed
    free(err_msg);

    wows_free_context(context);
    return ret;
}

/* Do something with the file */
wows_print_flat(context);

/* Free the context and munmap the index file */
wows_free_context(context);
```

# Build & development

## Dependencies

Installing the dependencies on Debian:

```shell
apt install cmake zlib1g-dev libpcre2-dev clang
```

Installing the test/coverage dependencies:

```shell
apt install lcov libcunit1-dev
```

## Build

```shell
cmake .
make
```

## Install

```shell
# Remove DESTDIR for system installation
make install DESTDIR=fakeroot
```
Files installed:
```shell
tree fakeroot
fakeroot
└── usr
    └── local
        ├── bin
        │   └── wows-depack-cli
        ├── include
        │   └── wows-depack.h
        └── lib
            ├── libwows-depack.so -> libwows-depack.so.0
            ├── libwows-depack.so.0 -> libwows-depack.so.0.1.0
            └── libwows-depack.so.0.1.0
```

## Test

To run the unit tests, do the following
```shell
# cmake run (on time)
cmake -DCOVERAGE=ON .
# run just the unit tests
make tests
# run also the coverage tests
make coverage
```

in case of issues, you can directly launch the test binary alone:
```shell
# directly:
./wows-depack-test

# or with gdb:
```

# Fuzzing

This tool/lib parses binary data, consequently checking for overflow might be a good idea.

To do so, install [american fuzzy lop++ (afl++)](https://aflplus.plus/):

```shell
# Debian/Ubuntu
apt install afl-clang

# Mac OS
brew install afl-fuzz
```

Compile with afl support:
```shell
cmake -DCMAKE_CXX_COMPILER=afl-clang++ -DCMAKE_C_COMPILER=afl-clang .
make
```

Run the fuzzing
```shell
# Replace with your WoWs install directory
INDEX_DIR="~/Games/World\ of\ Warships/bin/6775398/idx/"

afl-fuzz -i "$INDEX_DIR" -o ./out -t 10000 -- ./wows-depack-cli -i '@@'
```

Then wait for crashes to occure (hopefully this will be in vain).
