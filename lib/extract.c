#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include "wows-depack.h"
#include "wows-depack-private.h"
#include "hashmap.h"

int get_inode(WOWS_CONTEXT *context, char *path, WOWS_BASE_INODE **out_inode) {
    int dir_count;
    char **dirs;
    char *file;
    int ret = decompose_path(path, &dir_count, &dirs, &file);
    if (ret != 0) {
        return ret;
    }
    WOWS_DIR_INODE *inode = context->root;
    for (int i = 0; i < dir_count; i++) {
        if (inode != NULL && inode->type == WOWS_INODE_TYPE_DIR) {
            WOWS_DIR_INODE *inode_search = &(WOWS_DIR_INODE){.name = dirs[i]};
            struct hashmap *map = inode->children_inodes;
            inode = *(WOWS_DIR_INODE **)hashmap_get(map, &inode_search);
        }
        // We don't break here just to free each path items
        free(dirs[i]);
    }
    free(dirs);
    if (inode == NULL) {
        free(file);
        return WOWS_ERROR_NOT_FOUND;
    }
    WOWS_DIR_INODE *inode_search = &(WOWS_DIR_INODE){.name = file};
    struct hashmap *map = inode->children_inodes;
    inode = *(WOWS_DIR_INODE **)hashmap_get(map, &inode_search);

    free(file);
    if (inode == NULL) {
        return WOWS_ERROR_NOT_FOUND;
    }
    *out_inode = (WOWS_BASE_INODE *)inode;
    return 0;
}

int extract_file_inode(WOWS_CONTEXT *context, WOWS_FILE_INODE *file_inode, FILE *fp) {
    if (file_inode->type != WOWS_INODE_TYPE_FILE) {
        return WOWS_ERROR_NOT_A_FILE;
    }
    uint32_t idx_index = file_inode->index_file_index;
    WOWS_INDEX *index = context->indexes[idx_index];
    char *pkg_file_path;
    get_pkg_filepath(index, &pkg_file_path);
    printf("%s\n", pkg_file_path);
    free(pkg_file_path);
    return 0;
}

int wows_extract_file_fp(WOWS_CONTEXT *context, char *file_path, FILE *output) {
    WOWS_BASE_INODE *inode;
    int ret = get_inode(context, file_path, &inode);
    if (ret != 0) {
        return ret;
    }
    extract_file_inode(context, (WOWS_FILE_INODE *)inode, output);
    return 0;
}
