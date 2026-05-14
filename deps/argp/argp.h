#ifndef STANDALONE_ARGP_H
#define STANDALONE_ARGP_H

#include <stddef.h>
#include <stdio.h>

/* error_t is a GNU extension not present on BSD/macOS */
#ifndef __error_t_defined
typedef int error_t;
# define __error_t_defined 1
#endif

#define ARGP_ERR_UNKNOWN E2BIG
#define ARGP_KEY_ARG     0
#define ARGP_KEY_END     0x1000001
#define ARGP_KEY_NO_ARGS 0x1000002

/* Defined by the application */
extern const char *argp_program_version;
extern const char *argp_program_bug_address;

/* Forward declarations */
struct argp_state;

struct argp_option {
    const char *name;
    int         key;
    const char *arg;
    int         flags;
    const char *doc;
    int         group;
};

/* Forward declaration for struct argp */
struct argp_child;

struct argp {
    const struct argp_option *options;
    error_t (*parser)(int key, char *arg, struct argp_state *state);
    const char *args_doc;
    const char *doc;
    const struct argp_child *children;
    char *(*help_filter)(int key, const char *text, void *input);
    const char *argp_domain;
};

struct argp_state {
    const struct argp *root_argp;
    int                argc;
    char             **argv;
    int                next;
    unsigned           flags;
    void              *input;
    void             **child_inputs;
    void              *hook;
    char              *name;
    FILE              *err_stream;
    FILE              *out_stream;
    int                arg_num;
    int                quoted;
    void              *pstate;
    unsigned long      argnum;
};

extern error_t argp_parse(const struct argp *argp, int argc, char **argv,
                           unsigned flags, int *arg_index, void *input);
extern void argp_usage(const struct argp_state *state);
extern void argp_error(const struct argp_state *state, const char *fmt, ...);

#endif /* STANDALONE_ARGP_H */
