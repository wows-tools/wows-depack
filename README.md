# WoWs depack

[![Ubuntu-x86_64](https://github.com/kakwa/wows-depack/actions/workflows/ubuntu-x86_64.yml/badge.svg)](https://github.com/kakwa/wows-depack/actions/workflows/ubuntu-x86_64.yml)
[![Coverage Status](https://coveralls.io/repos/github/kakwa/wows-depack/badge.svg?branch=main)](https://coveralls.io/github/kakwa/wows-depack?branch=main)

World of Warships resource files unpacker

The format specification is presented in [FORMAT.md](https://github.com/kakwa/wows-depack/blob/main/FORMAT.md).

The reverse engineering story is detailed in [REVERSING_STORY.md](https://github.com/kakwa/wows-depack/blob/main/REVERSING_STORY.md)

# Usage

## CLI tool

### Help
```shell
./wows-depack-cli --help

Usage: wows-depack-cli [OPTION...] <-i INPUT_FILE|-I INPUT_DIR>

World of Warships resource extractor tool

  -e, --extract=FILE_DIR_TO_EXTRACT
                             File or directory to extract
  -i, --input=INPUT_INDEX    Input file
  -I, --input-dir=INPUT_INDEX_DIR
                             Input file
  -o, --output=OUTPUT_FILE   Output file when extracting one file
  -O, --output-dir=OUTPUT_DIR   Output dir for recursive extract
  -p, --print                Print All files
  -s, --search=SEARCH_PATTERN   Search Regex
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to https://github.com/kakwa/wows-depack/issues.
```

### Parse the index(es)

Parse a single Index file:

```shell
./wows-depack-cli -i ~/Games/World\ of\ Warships/bin/6831266/idx/system_data.idx
```

Parse all the indexes in the index directory:

```shell
./wows-depack-cli -I ~/Games/World\ of\ Warships/bin/6831266/idx/
```

Note, the other examples are using `-I <DIR>` , but it could be faster to specify one index file directly through `-i <FILE>`.
Specifically, `GameParams,data` is referenced in the `system_data.idx` index.

Also, please note that you will need to adapt the path to your WoWs install and the parent of `idx/` (the `6831266` part changes with every update).

### Print all files

To print all the files present:
```shell
./wows-depack-cli -I ~/Games/World\ of\ Warships/bin/6831266/idx/ -p

/gui/modernization_icons/icon_modernization_PCM020_DamageControl_Mod_I.png
/gui/modernization_icons/icon_modernization_PCM047_Special_Mod_I_Montana.png
[...]
/clanbase/headquarters_2/6/29/31.png
/server_stats.xml

```
### Search

To search a given file (regular expression):

```shell
./wows-depack-cli -I ~/Games/World\ of\ Warships/bin/6831266/idx -s '.*Params.*'

Found 3 matching files:
shipyardParams.xml
content/GameParams.data
content/UIParams.data
```

### Extract A single file

To Extract a Single file, do:

```shell
./wows-depack-cli -I ~/Games/World\ of\ Warships/bin/6831266/idx \
	-e 'content/GameParams.data' -o GameParams.data
```

### Extract a whole directory

To extract a whole sub directory, do:

```shell
./wows-depack-cli -I ~/Games/World\ of\ Warships/bin/6775398/idx/ \
	-e 'content/' -O out/
```

To extract everything (rip your free disk space), do:

```shell
./wows-depack-cli -I ~/Games/World\ of\ Warships/bin/6775398/idx/ \
	-e '/' -O out/
```

## Library

### API documentation

The API documentation [is available here](https://kakwa.github.io/wows-depack/wows-depack_8h.html).

### Usage examples

#### Initialize Context & Parse Index Files

To start searching/extracting file, you first need to init & fill the context.

With a single index file:

```C
#include "wows-depack.h"


/* Other debug categories */
// WOWS_DEBUG_RAW_RECORD
// WOWS_DEBUG_FILE_LISTING
/* several debug categories can be enabled like so: */
// WOWS_DEBUG_FILE_LISTING | WOWS_DEBUG_RAW_RECORD;


WOWS_CONTEXT *context = wows_init_context(WOWS_NO_DEBUG);

/* Parse the index file */
int ret = wows_parse_index(index_file_path, context);
```

To parse the indexes, you can either specify a specific index file:

```C
char *index_file_path = "path/to/file.idx"

/* Parse the index file */
int ret = wows_parse_index(index_file_path, context);
```

Alternatively, you can parse all the index files in a given directory:

```C
char *index_dir_path = "path/to/index/dir/"

/* Parse the index file */
int ret = wows_parse_index_dir(index_dir_path, context);
```

These operations will parse one or several index files, and construct a pseudo-filesystem tree which can be then explored.

#### Searching files/directories

It's possible to search for matching files inside the indexes:

```C
// Init output variables
int resc;
char **res_files;


/* Supported search modes:
WOWS_SEARCH_FILE_ONLY     < Search only on file names.
WOWS_SEARCH_DIR_ONLY      < Search only on directory names.
WOWS_SEARCH_FILE_PLUS_DIR < Search on directory and file names.
WOWS_SEARCH_FULL_PATH     < Search on the full path of files.
*/

// PCRE search pattern, please note that the final regex is "^<search_pattern>$"
char *search_pattern = ".*Params.*";

// Search the files
wows_search(context, search_pattern, WOWS_SEARCH_FILE_ONLY, &resc, &res_files);

// Print and free the result
printf("Found %d matching files:\n", resc);
for (int i = 0; i < resc; i++) {
    printf("%s\n", res_files[i]);
    // Free Each file path must be freed
    free(res_files[i]);
}
// Free the array containing these paths
free(res_files);
```

#### Extracting Files

To extract files, you can do the following:

```C
char *output = "path/output.xml"

// Extract to output file
ret = wows_extract_file(context, "stuff.xml", output);
```

If you want to control the underlying File Pointer (for example, to write in a memstream), extract functions also provide an `_fp` version whenever possible:

```C
char *buf = NULL;
size_t buf_size = 0;

// Open the file
FILE *f = open_memstream(&buf, &buf_size);

// Extract to File *
ret = wows_extract_file_fp(context, "stuff.xml", f);

// Close the file
fclose(f);

free(buff);
```

You can also extract files recursively:

```C
TODO
```

#### Clean-up

Once done, you need to release the context memory:

```C
wows_free_context(context);
```

#### Error Handling & Debugging

Most `wows_*` returns 0 on success or error codes on failure.
To convert it to an error message, you can do the following:

```C
// wows_* function call example
int ret = wows_parse_index(index_file_path, context);

/* Error handling */
if (ret != 0) {
    /* get an error message + additional info from context + return code */
    char *err_msg = wows_error_string(ret, context);
    printf("Error: %s\n", err_msg);
    // the message must be freed
    free(err_msg);
}
```

To print the whole pseudo-filesystem tree to `stdout`, you can use the following functions:

```C
// Print a tree like layout
wows_print_tree(context);

// Print the full path of each file, one per line
wows_print_flat(context);
```

#### Write support

wows-depack provides experimental write support for creating index and pkg files:

```C
char *input_dir = "./tests";
FILE *nfd_pkg = fopen("stuff.pkg", "w+");
FILE *nfd_idx = fopen("stuff.idx", "w+");
wows_write_pkg(context, input_dir, "stuff.pkg", nfd_pkg, nfd_idx);
fclose(nfd_idx);
fclose(nfd_pkg);
```

Please note that the write support is limited, and was created mainly to generate test data.

# Build & development

## Dependencies

Installing the dependencies on Debian:

```shell
apt install cmake zlib1g-dev libpcre2-dev clang
```

Installing the test/coverage/doc dependencies:

```shell
apt install lcov libcunit1-dev doxygen
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

## Tests

To run the unit tests, do the following:

```shell
# cmake run (on time)
cmake -DCOVERAGE=ON -DBUILD_TESTS=ON .
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
gdb --args ./wows-depack-test
```

## Documentation

To build the doxygen documentation:

```shell
cmake -DBUILD_DOC=ON .

make doc_doxygen
```

Optionally, get a nicer doxygen theme:

```shell
./misc/setup_doxycss.sh

make doc_doxygen
```

## Fuzzing

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

Then wait for crashes to occur (hopefully this will be in vain).
