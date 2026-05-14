#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <errno.h>
#include "argp.h"

void argp_usage(const struct argp_state *state) {
    fprintf(stderr, "Try `%s --help' for more information.\n",
            state->name ? state->name : "program");
    exit(1);
}

void argp_error(const struct argp_state *state, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "%s: ", state->name ? state->name : "program");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    argp_usage(state);
}

static void print_help(const struct argp *ap, const char *prog_name) {
    fprintf(stdout, "Usage: %s [OPTION...] %s\n",
            prog_name, ap->args_doc ? ap->args_doc : "");
    if (ap->doc && *ap->doc)
        fprintf(stdout, "%s\n", ap->doc);
    fprintf(stdout, "\n");

    const struct argp_option *opt = ap->options;
    while (opt->name || opt->key || opt->doc) {
        if (!opt->name && !opt->key) {
            if (opt->doc)
                fprintf(stdout, "\n %s:\n", opt->doc);
        } else {
            char left[40];
            if (opt->key > 32 && opt->key < 127) {
                if (opt->arg)
                    snprintf(left, sizeof(left), "-%c, --%s=%s",
                             opt->key, opt->name ? opt->name : "", opt->arg);
                else
                    snprintf(left, sizeof(left), "-%c, --%s",
                             opt->key, opt->name ? opt->name : "");
            } else {
                if (opt->arg)
                    snprintf(left, sizeof(left), "    --%s=%s",
                             opt->name ? opt->name : "", opt->arg);
                else
                    snprintf(left, sizeof(left), "    --%s",
                             opt->name ? opt->name : "");
            }
            fprintf(stdout, "  %-32s %s\n", left, opt->doc ? opt->doc : "");
        }
        opt++;
    }
    fprintf(stdout, "  -?, -h, --help                   Give this help list\n");
    if (argp_program_version)
        fprintf(stdout, "  -V, --version                    Print program version\n");
    if (argp_program_bug_address)
        fprintf(stdout, "\nReport bugs to %s.\n", argp_program_bug_address);
}

error_t argp_parse(const struct argp *ap, int argc, char **argv,
                   unsigned flags, int *arg_index, void *input) {
    /* Count entries that have an actual option key */
    int n = 0;
    for (const struct argp_option *o = ap->options; o->name || o->key || o->doc; o++) {
        if (o->key)
            n++;
    }

    /* +3 for --help, --version, null terminator */
    struct option *long_opts = calloc((size_t)(n + 3), sizeof(struct option));
    /* ':' prefix + 2 chars per option + 'h' + 'V' + NUL */
    char *short_opts = calloc((size_t)(n * 2 + 6), 1);
    int li = 0, si = 0;

    /* Leading ':' suppresses getopt's own error messages */
    short_opts[si++] = ':';

    for (const struct argp_option *o = ap->options; o->name || o->key || o->doc; o++) {
        if (!o->key)
            continue;
        if (o->name) {
            long_opts[li].name    = o->name;
            long_opts[li].has_arg = o->arg ? required_argument : no_argument;
            long_opts[li].flag    = NULL;
            long_opts[li].val     = o->key;
            li++;
        }
        short_opts[si++] = (char)o->key;
        if (o->arg)
            short_opts[si++] = ':';
    }

    /* Add built-in --help and --version */
    long_opts[li++] = (struct option){"help",    no_argument, NULL, 'h'};
    short_opts[si++] = 'h';
    if (argp_program_version) {
        long_opts[li++] = (struct option){"version", no_argument, NULL, 'V'};
        short_opts[si++] = 'V';
    }
    /* long_opts[li] already zeroed (terminator) */

    struct argp_state state = {0};
    state.root_argp  = ap;
    state.input      = input;
    state.name       = argv[0];
    state.err_stream = stderr;
    state.out_stream = stdout;

    int c;
    error_t err = 0;
    while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (c) {
        case 'h':
        case '?':
            if (c == 'h' || optopt == 0) {
                print_help(ap, argv[0]);
                free(long_opts);
                free(short_opts);
                exit(0);
            }
            fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], optopt);
            fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
            free(long_opts);
            free(short_opts);
            exit(1);
        case ':':
            fprintf(stderr, "%s: option requires an argument -- '%c'\n",
                    argv[0], optopt);
            free(long_opts);
            free(short_opts);
            exit(1);
        case 'V':
            fprintf(stdout, "%s\n",
                    argp_program_version ? argp_program_version : "unknown");
            free(long_opts);
            free(short_opts);
            exit(0);
        default:
            err = ap->parser(c, optarg, &state);
            if (err != 0 && err != ARGP_ERR_UNKNOWN) {
                free(long_opts);
                free(short_opts);
                return err;
            }
            break;
        }
    }

    if (arg_index)
        *arg_index = optind;

    free(long_opts);
    free(short_opts);
    return 0;
}
