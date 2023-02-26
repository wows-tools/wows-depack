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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
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

const char *argp_program_bug_address = "https://github.com/kakwa/wows-depack/issues";

static char doc[] = "\nWorld of Warships resource extractor tool";

static struct argp_option options[] = {{"input", 'i', "INPUT_INDEX", 0, "Input file"},
                                       {"input-dir", 'I', "INPUT_INDEX_DIR", 0, "Input file"},
                                       //    {"output-dir", 'O', "OUTPUT_DIR", 0, "Output dir with the decompressed
                                       //    blobs"},
                                       {0}};

/* A description of the arguments we accept. */
static char args_doc[] = "-i INPUT_FILE -o OUTPUT_DIR";

struct arguments {
    char *args[2]; /* arg1 & arg2 */
    char *output;
    char *input;
    char *input_dir;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = (struct arguments *)state->input;

    switch (key) {
    case 'i':
        arguments->input = arg;
        break;
    case 'I':
        arguments->input_dir = arg;
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

    if (args.input == NULL && args.input_dir == NULL) {
        fprintf(stderr, "error: no -i <input file> or -I <input dir> arg\n");
        return EXIT_FAILURE;
    }

    WOWS_CONTEXT *context = wows_init_context(NO_DEBUG);
    // context->debug_level = DEBUG_FILE_LISTING | DEBUG_RAW_RECORD;
    // context->debug_level = DEBUG_RAW_RECORD;
    // context->debug_level = DEBUG_FILE_LISTING;

    int ret = 0;
    if (args.input != NULL) {
        ret = wows_parse_index_file(args.input, context);
    }
    if (args.input_dir != NULL) {
        ret = wows_parse_index_dir(args.input_dir, context);
    }
    if (ret != 0) {
        char *err_msg = wows_error_string(ret, context);
        printf("Error: %s\n", err_msg);
        free(err_msg);
        wows_free_context(context);
        return ret;
    }
    // wows_print_tree(context);

    int resc;
    char **res_files;
    wows_search(context, ".*Params.*", WOWS_SEARCH_FILE_ONLY, &resc, &res_files);
    printf("Found %d matching files:\n", resc);
    for (int i = 0; i < resc; i++) {
        printf("%s\n", res_files[i]);
        free(res_files[i]);
    }
    free(res_files);
    // wows_print_flat(context);
    wows_free_context(context);
    return 0;
}
