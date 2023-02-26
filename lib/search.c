#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wows-depack.h"
#include "wows-depack-private.h"
#include "hashmap.h"
#include <pcre.h>

#define SEARCH_RESULT_ALLOC_SIZE 10

typedef struct {
    WOWS_CONTEXT *context;
    char *pattern;
    pcre *matcher;
    int mode;
    char **results;
    int result_count;
} search_context;

bool search_inode(const void *item, void *udata) {
    WOWS_BASE_INODE *inode = *(WOWS_BASE_INODE **)item;
    search_context *ctx = (search_context *)udata;

    if (inode->type == WOWS_INODE_TYPE_FILE) {
        WOWS_FILE_INODE *file_inode = (WOWS_FILE_INODE *)inode;
        char **parent_entries = calloc(WOWS_DIR_MAX_LEVEL, sizeof(char *));
        int depth;
        get_path_inode(file_inode, &depth, parent_entries);

        if (ctx->result_count % SEARCH_RESULT_ALLOC_SIZE == SEARCH_RESULT_ALLOC_SIZE - 1) {
            ctx->results = realloc(ctx->results, sizeof(char *) * (ctx->result_count + SEARCH_RESULT_ALLOC_SIZE));
        }
        if (match_regex(ctx->matcher, file_inode->name)) {
            // pattern found in the filename, add it to the results
            char *path = join_path(parent_entries, depth, file_inode->name);
            ctx->results[ctx->result_count++] = path;
        }
        free(parent_entries);
    } else if (inode->type == WOWS_INODE_TYPE_DIR) {
        // recursively search the directory
        WOWS_DIR_INODE *dir_inode = (WOWS_DIR_INODE *)inode;
        struct hashmap *map = dir_inode->children_inodes;
        hashmap_scan(map, search_inode, udata);
    }
    return true;
}

int wows_search(WOWS_CONTEXT *context, char *pattern, int mode, int *result_count, char ***results) {
    search_context ctx = {.context = context,
                          .pattern = pattern,
                          .mode = mode,
                          .results = calloc(SEARCH_RESULT_ALLOC_SIZE, sizeof(char *)),
                          .result_count = 0};

    char *pattern_start_end = calloc(sizeof(char), strlen(pattern) + 3);
    // Add ^ and $ around the search pattern for strict matching
    memcpy(pattern_start_end + 1, pattern, strlen(pattern));
    pattern_start_end[0] = '^';
    pattern_start_end[strlen(pattern) + 1] = '$';

    ctx.matcher = compile_regex(pattern_start_end);
    if (ctx.matcher == NULL) {
        return WOWS_ERROR_INVALID_SEARCH_PATTERN;
    }

    WOWS_DIR_INODE *root = context->root;
    hashmap_scan(root->children_inodes, search_inode, &ctx);

    *result_count = ctx.result_count;
    *results = ctx.results;

    free_regex(ctx.matcher);
    free(pattern_start_end);
    return 0;
}
