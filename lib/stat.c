#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wows-depack.h"
#include "wows-depack-private.h"
#include "hashmap.h"

#define READDIR_ALLOC_SIZE 16

typedef struct {
    char **entries;
    int entry_count;
} readdir_ctx;

static bool collect_child_name(const void *item, void *udata) {
    WOWS_BASE_INODE *inode = *(WOWS_BASE_INODE **)item;
    readdir_ctx *ctx = (readdir_ctx *)udata;

    if (ctx->entry_count % READDIR_ALLOC_SIZE == READDIR_ALLOC_SIZE - 1) {
        ctx->entries = realloc(ctx->entries, sizeof(char *) * (ctx->entry_count + READDIR_ALLOC_SIZE));
    }
    ctx->entries[ctx->entry_count++] = strdup(inode->name);
    return true;
}

int wows_readdir(WOWS_CONTEXT *context, char *dir_path, int *entry_count, char ***entries) {
    WOWS_BASE_INODE *inode;
    int ret = get_inode(context, dir_path, &inode);
    if (ret != 0)
        return ret;
    if (inode->type != WOWS_INODE_TYPE_DIR) {
        wows_set_error_details(context, "'%s' is not a directory", dir_path);
        return WOWS_ERROR_NOT_A_DIR;
    }

    WOWS_DIR_INODE *dir_inode = (WOWS_DIR_INODE *)inode;
    readdir_ctx ctx = {
        .entries = calloc(READDIR_ALLOC_SIZE, sizeof(char *)),
        .entry_count = 0,
    };

    hashmap_scan(dir_inode->children_inodes, collect_child_name, &ctx);

    *entry_count = ctx.entry_count;
    *entries = ctx.entries;
    return 0;
}

int wows_stat_path(WOWS_CONTEXT *context, char *path, WOWS_STAT *stat) {
    WOWS_BASE_INODE *inode;
    int ret = get_inode(context, path, &inode);
    if (ret != 0)
        return ret;

    stat->type = inode->type;

    if (inode->type == WOWS_INODE_TYPE_FILE) {
        WOWS_INDEX_DATA_FILE_ENTRY *entry_search = &(WOWS_INDEX_DATA_FILE_ENTRY){.metadata_id = inode->id};
        void *res = hashmap_get(context->file_map, &entry_search);
        if (res == NULL) {
            wows_set_error_details(context, "missing metadata for file '%s'", path);
            return WOWS_ERROR_MISSING_METADATA_ENTRY;
        }
        WOWS_INDEX_DATA_FILE_ENTRY *entry = *(WOWS_INDEX_DATA_FILE_ENTRY **)res;
        stat->size = entry->size_pkg_data;
    } else {
        stat->size = 0;
    }
    return 0;
}
