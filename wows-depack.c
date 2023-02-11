/* Heavily based on zpipe.c from zlib */

/* zpipe.c: example of proper use of zlib's inflate() and deflate()
   Not copyrighted -- provided to the public domain
   Version 1.4  11 December 2005  Mark Adler */

#define _POSIX_C_SOURCE 200809L

#include <argp.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>
#include "inc/wows-depack.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

const char *argp_program_version = BFD_VERSION;

const char *argp_program_bug_address =
    "Pierre-Francois Carpentier <carpentier.pf@gmail.com>";

static char doc[] = "\nBrute force DEFLATE decompressor";

static struct argp_option options[] = {
    {"input", 'i', "INPUT_INDEX", 0, "Input file"},
    {"input-dir", 'I', "INPUT_INDEX_DIR", 0, "Input file"},
    {"output", 'o', "OUTPUT_DIR", 0, "Output dir with the decompressed blobs"},
    {0}};

/* A description of the arguments we accept. */
static char args_doc[] = "-i INPUT_FILE -o OUTPUT_DIR";

struct arguments {
  char *args[2]; /* arg1 & arg2 */
  char *output;
  char *input;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = (struct arguments *)state->input;

  switch (key) {
  case 'i':
    arguments->input = arg;
    break;
  case 'o':
    arguments->output = arg;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char **argv) {
  struct arguments args;
  args.input = NULL;
  args.output = NULL;
  argp_parse(&argp, argc, argv, 0, 0, &args);

  if (args.input == NULL) {
    fprintf(stderr, "error: no -i <input file> arg\n");
    return EXIT_FAILURE;
  }

  if (args.output == NULL) {
    fprintf(stderr, "error: no -o <output dir> arg\n");
    return EXIT_FAILURE;
  }
  FILE *in = fopen(args.input, "r");
  if (in == NULL) {
    fprintf(stderr, "error: failed to open '%s'\n", args.input);
    return EXIT_FAILURE;
  }

  int res = mkdir(args.output, 0700);
  if (res != 0 && !wows_is_dir(args.output)) {
    fprintf(stderr, "error: failed to create output '%s' dir: %s\n",
            args.output, strerror(errno));
    return EXIT_FAILURE;
  } else {
  }
  /* avoid end-of-line conversions */
  // SET_BINARY_MODE(stdin);
  // SET_BINARY_MODE(stdout);

  return wows_inflate_all(in, args.output);
}
