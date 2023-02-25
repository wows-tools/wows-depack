#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wows-depack.h"
#include "wows-depack-private.h"
#include "hashmap.h"

bool dir_inode_search(const void *item, void *udata) {
    search_inode(*(WOWS_BASE_INODE **)item);
    return true;
}

int search_inode(WOWS_BASE_INODE *inode) {
    if (inode->type == WOWS_INODE_TYPE_FILE) {
        WOWS_FILE_INODE *file_inode = (WOWS_FILE_INODE *)inode;
        int depth;
        char **parent_entries = calloc(WOWS_DIR_MAX_LEVEL, sizeof(char *));
        get_path_inode(file_inode, &depth, parent_entries);

        printf("/");
        for (int j = (depth - 1); j > -1; j--) {
            printf("%s/", parent_entries[j]);
        }
        printf("%s\n", file_inode->name);
        free(parent_entries);
    }
    if (inode->type == WOWS_INODE_TYPE_DIR) {
        WOWS_DIR_INODE *dir_inode = (WOWS_DIR_INODE *)inode;
        struct hashmap *map = dir_inode->children_inodes;
        hashmap_scan(map, dir_inode_search, NULL);
    }
    return 0;
}

int wows_search(WOWS_CONTEXT *context, char *pattern, int mode, int *result_count, char ***results) {
    search_inode(context->root);
    return 0;
}
