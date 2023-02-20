#define _POSIX_C_SOURCE 200809L
// TODO clean-up this mess
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

#include "wows-depack.h"
#include "wows-depack-private.h"
#include "hashmap.h"

int metadata_compare(const void *a, const void *b, void *udata) {
    const WOWS_INDEX_METADATA_ENTRY *ma = *(WOWS_INDEX_METADATA_ENTRY **)a;
    const WOWS_INDEX_METADATA_ENTRY *mb = *(WOWS_INDEX_METADATA_ENTRY **)b;
    return ma->id > mb->id;
}

uint64_t metadata_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const WOWS_INDEX_METADATA_ENTRY *meta = *(WOWS_INDEX_METADATA_ENTRY **)item;
    return meta->id;
}

int file_compare(const void *a, const void *b, void *udata) {
    const WOWS_INDEX_DATA_FILE_ENTRY *fa = *(WOWS_INDEX_DATA_FILE_ENTRY **)a;
    const WOWS_INDEX_DATA_FILE_ENTRY *fb = *(WOWS_INDEX_DATA_FILE_ENTRY **)b;
    return fa->metadata_id > fb->metadata_id;
}

uint64_t file_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const WOWS_INDEX_DATA_FILE_ENTRY *file = *(WOWS_INDEX_DATA_FILE_ENTRY **)item;
    return file->metadata_id;
}

int dir_inode_compare(const void *a, const void *b, void *udata) {
    const WOWS_BASE_INODE *ia = *(WOWS_BASE_INODE **)a;
    const WOWS_BASE_INODE *ib = *(WOWS_BASE_INODE **)b;
    return strcmp(ia->name, ib->name);
}

uint64_t dir_inode_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const WOWS_BASE_INODE *inode = *(WOWS_BASE_INODE **)item;
    return hashmap_sip(inode->name, strlen(inode->name), seed0, seed1);
}

int get_metadata_filename(WOWS_INDEX_METADATA_ENTRY *entry, WOWS_INDEX *index, char **out) {
    char *filename = (char *)entry;
    filename += entry->offset_idx_file_name;
    // Check that the file name is not too long
    if (entry->file_name_size > WOWS_PATH_MAX) {
        return WOWS_ERROR_PATH_TOO_LONG;
    }
    // Check that the string is actually within the index boundaries
    returnOutIndex(filename, filename + entry->file_name_size, index);
    // Check that it is null terminated
    if (filename[entry->file_name_size - 1] != '\0') {
        return WOWS_ERROR_NON_ZERO_TERMINATED_STRING;
    }
    *out = filename;
    return 0;
}

int get_footer_filename(WOWS_INDEX_FOOTER *footer, WOWS_INDEX *index, char **out) {
    char *pkg_filename = (char *)footer;
    pkg_filename += sizeof(WOWS_INDEX_FOOTER);
    // Check that the file name is not too long
    if (footer->pkg_file_name_size > WOWS_PATH_MAX) {
        return WOWS_ERROR_PATH_TOO_LONG;
    }
    // Check that the string is actually within the index boundaries
    returnOutIndex(pkg_filename, pkg_filename + footer->pkg_file_name_size, index);
    // Check that it is null terminated
    if (pkg_filename[footer->pkg_file_name_size - 1] != '\0') {
        return WOWS_ERROR_NON_ZERO_TERMINATED_STRING;
    }
    *out = pkg_filename;
    return 0;
}

int add_child_inode(WOWS_DIR_INODE *parent_inode, WOWS_BASE_INODE *inode) {
    hashmap_set(parent_inode->children_inodes, &inode);
    return 0;
}

WOWS_BASE_INODE *get_child(WOWS_DIR_INODE *inode, char *name) {
    WOWS_BASE_INODE *ientry_search = &(WOWS_BASE_INODE){.name = name};
    void *res = hashmap_get(inode->children_inodes, &ientry_search);
    if (res == NULL) {
        return NULL;
    }
    return *(WOWS_BASE_INODE **)res;
}

WOWS_DIR_INODE *init_root_inode() {
    WOWS_DIR_INODE *dir_inode = calloc(sizeof(WOWS_DIR_INODE), 1);
    dir_inode->type = WOWS_INODE_TYPE_DIR;
    dir_inode->id = WOWS_ROOT_INODE;
    dir_inode->index_file_index = WOWS_ROOT_INODE;
    dir_inode->parent_inode = NULL;
    dir_inode->name = ".";
    dir_inode->children_inodes =
        hashmap_new(sizeof(WOWS_BASE_INODE *), 0, 0, 0, dir_inode_hash, dir_inode_compare, NULL, NULL);
    return dir_inode;
}

WOWS_DIR_INODE *init_dir_inode(uint64_t metadata_id, uint32_t current_index_context, WOWS_DIR_INODE *parent_inode,
                               WOWS_CONTEXT *context) {
    char *name;
    WOWS_INDEX_METADATA_ENTRY *mentry_search = &(WOWS_INDEX_METADATA_ENTRY){.id = metadata_id};
    void *res = hashmap_get(context->metadata_map, &mentry_search);
    if (res == NULL) {
        return NULL;
    }
    WOWS_INDEX_METADATA_ENTRY *entry = *(WOWS_INDEX_METADATA_ENTRY **)res;
    int ret = get_metadata_filename(entry, context->indexes[current_index_context], &name);
    if (ret != 0) {
        return NULL;
    }

    WOWS_DIR_INODE *dir_inode = calloc(sizeof(WOWS_DIR_INODE), 1);
    dir_inode->type = WOWS_INODE_TYPE_DIR;
    dir_inode->id = metadata_id;
    dir_inode->index_file_index = current_index_context;
    dir_inode->parent_inode = parent_inode;
    dir_inode->name = name;
    dir_inode->children_inodes =
        hashmap_new(sizeof(WOWS_BASE_INODE *), 0, 0, 0, dir_inode_hash, dir_inode_compare, NULL, NULL);

    if (parent_inode != NULL) {
        add_child_inode(parent_inode, (WOWS_BASE_INODE *)dir_inode);
    } else {
        free(dir_inode);
        return NULL;
    }
    return dir_inode;
}

WOWS_FILE_INODE *init_file_inode(uint64_t metadata_id, uint32_t current_index_context, WOWS_DIR_INODE *parent_inode,
                                 WOWS_CONTEXT *context) {
    WOWS_INDEX_METADATA_ENTRY *mentry_search = &(WOWS_INDEX_METADATA_ENTRY){.id = metadata_id};
    void *res = hashmap_get(context->metadata_map, &mentry_search);
    if (res == NULL) {
        return NULL;
    }
    WOWS_INDEX_METADATA_ENTRY *entry = *(WOWS_INDEX_METADATA_ENTRY **)res;
    char *name;
    int ret = get_metadata_filename(entry, context->indexes[current_index_context], &name);
    if (ret != 0) {
        return NULL;
    }

    WOWS_FILE_INODE *file_inode = calloc(sizeof(WOWS_FILE_INODE), 1);
    file_inode->type = WOWS_INODE_TYPE_FILE;
    file_inode->id = metadata_id;
    file_inode->name = name;
    file_inode->index_file_index = current_index_context;
    file_inode->parent_inode = parent_inode;
    if (parent_inode != NULL) {
        add_child_inode(parent_inode, (WOWS_BASE_INODE *)file_inode);
    } else {
        free(file_inode);
        return NULL;
    }
    return file_inode;
}

// Context init function
WOWS_CONTEXT *wows_init_context(uint8_t debug_level) {
    WOWS_CONTEXT *context = calloc(sizeof(WOWS_CONTEXT), 1);
    context->metadata_map =
        hashmap_new(sizeof(WOWS_INDEX_METADATA_ENTRY *), 0, 0, 0, metadata_hash, metadata_compare, NULL, NULL);
    context->file_map = hashmap_new(sizeof(WOWS_INDEX_METADATA_ENTRY *), 0, 0, 0, file_hash, file_compare, NULL, NULL);
    context->debug_level = debug_level;
    context->root = init_root_inode();
    return context;
}

// Map the different section of the index file content to an index struct
int map_index_file(char *contents, size_t length, WOWS_INDEX **index_in) {
    WOWS_INDEX *index = calloc(sizeof(WOWS_INDEX), 1);
    index->start_address = contents;
    index->end_address = contents + length;
    index->length = length;

    // Get the header section
    WOWS_INDEX_HEADER *header = (WOWS_INDEX_HEADER *)contents;
    index->header = header;
    // Check header section boundaries
    returnOutIndex((char *)header, contents + sizeof(WOWS_INDEX_HEADER), index);

    // Check that the magic do match
    if (strncmp(header->magic, "ISFP", 4) != 0) {
        return WOWS_ERROR_BAD_MAGIC;
    }

    // Get the start of the metadata array
    WOWS_INDEX_METADATA_ENTRY *metadatas;
    metadatas = (WOWS_INDEX_METADATA_ENTRY *)(contents + sizeof(WOWS_INDEX_HEADER));
    index->metadata = metadatas;
    // Check metadatas section boundaries
    returnOutIndex((char *)metadatas, (char *)&metadatas[header->file_dir_count], index);

    // Get the start pkg data pointer section
    WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry =
        (WOWS_INDEX_DATA_FILE_ENTRY *)(contents + header->offset_idx_data_section + MAGIC_SECTION_OFFSET);
    index->data_file_entry = data_file_entry;
    // Check data_file_entries section boundaries
    returnOutIndex((char *)data_file_entry, (char *)&data_file_entry[header->file_count], index);

    // Get the footer section
    WOWS_INDEX_FOOTER *footer =
        (WOWS_INDEX_FOOTER *)(contents + header->offset_idx_footer_section + MAGIC_SECTION_OFFSET);
    index->footer = footer;
    // Check footer section boundaries
    returnOutIndex((char *)footer, (char *)footer + sizeof(WOWS_INDEX_FOOTER), index);
    *index_in = index;
    return 0;
}

// Add the parsed index to the context list
uint32_t add_index_context(WOWS_CONTEXT *context, WOWS_INDEX *index) {
    context->indexes = realloc(context->indexes, context->index_count + 1);
    context->indexes[context->index_count] = index;
    context->index_count++;
    return context->index_count - 1;
}

// Get the dir/subdir path of a given file entry
int get_path(WOWS_CONTEXT *context, WOWS_INDEX_METADATA_ENTRY *mentry, int *depth,
             WOWS_INDEX_METADATA_ENTRY **entries) {
    struct hashmap *map = context->metadata_map;
    int level = 0;
    *depth = level;

    WOWS_INDEX_METADATA_ENTRY *mentry_search;

    WOWS_INDEX_METADATA_ENTRY *m_parent_entry_search = &(WOWS_INDEX_METADATA_ENTRY){.id = mentry->parent_id};
    WOWS_INDEX_METADATA_ENTRY **mparent_entry = (WOWS_INDEX_METADATA_ENTRY **)hashmap_get(map, &m_parent_entry_search);

    while (mparent_entry != NULL && level < WOWS_DIR_MAX_LEVEL) {
        entries[level] = *mparent_entry;
        mentry_search = &(WOWS_INDEX_METADATA_ENTRY){.id = (*mparent_entry)->parent_id};
        mparent_entry = NULL;
        mparent_entry = (WOWS_INDEX_METADATA_ENTRY **)hashmap_get(map, &mentry_search);
        level++;
    }
    *depth = level;
    return 0;
}

int wows_parse_index(char *index_file_path, WOWS_CONTEXT *context) {
    // Open the index file
    int fd = open(index_file_path, O_RDONLY);
    if (fd <= 0) {
        return WOWS_ERROR_FILE_OPEN_FAILURE;
    }

    // Recover the file size
    struct stat s;
    fstat(fd, &s);
    /* index content size */
    size_t length = s.st_size;

    // Map the whole content in memory
    char *content = mmap(0, length, PROT_READ, MAP_PRIVATE, fd, 0);
    return wows_parse_index_buffer(content, length, index_file_path, context);
}

// Index Parser function
int wows_parse_index_buffer(char *contents, size_t length, char *index_file_path, WOWS_CONTEXT *context) {
    int i;

    WOWS_INDEX *index;
    int err = map_index_file(contents, length, &index);
    index->index_file_path = index_file_path;

    if (err != 0) {
        return err;
    }

    char *filler;

    // Index all the metadata entries into an hmap
    for (i = 0; i < index->header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *entry = &index->metadata[i];
        // Check that we can correctly recover the file names
        int ret = get_metadata_filename(entry, index, &filler);
        if (ret != 0) {
            return ret;
        }
        hashmap_set(context->metadata_map, &entry);
    }

    // Check that we can correctly recover the file names
    int ret = get_footer_filename(index->footer, index, &filler);
    if (ret != 0) {
        return ret;
    }

    uint32_t current_index_context = add_index_context(context, index);

    // Debugging output if necessary
    if (context->debug_level & DEBUG_RAW_RECORD) {
        print_debug_raw(index);
    }
    if (context->debug_level & DEBUG_FILE_LISTING) {
        print_debug_files(index, context->metadata_map);
    }

    // Construct the inode tree
    for (i = 0; i < index->header->file_count; i++) {
        WOWS_INDEX_DATA_FILE_ENTRY *fentry = &index->data_file_entry[i];
        hashmap_set(context->file_map, &fentry);
        WOWS_INDEX_METADATA_ENTRY *mentry_search = &(WOWS_INDEX_METADATA_ENTRY){.id = fentry->metadata_id};

        void *res = hashmap_get(context->metadata_map, &mentry_search);
        if (res == NULL) {
            return WOWS_ERROR_MISSING_METADATA_ENTRY;
        }
        WOWS_INDEX_METADATA_ENTRY *mentry = *(WOWS_INDEX_METADATA_ENTRY **)res;

        // Get the path branch (all the parent directory branch)
        int depth;
        WOWS_INDEX_METADATA_ENTRY **parent_entries;
        parent_entries = calloc(WOWS_DIR_MAX_LEVEL, sizeof(WOWS_INDEX_METADATA_ENTRY *));
        get_path(context, mentry, &depth, parent_entries);

        // Insert the parent directories if necessary
        WOWS_DIR_INODE *parent_inode = context->root;
        for (int j = (depth - 1); j > -1; j--) {
            char *name;
            get_metadata_filename(parent_entries[j], index, &name);
            WOWS_DIR_INODE *existing_inode = (WOWS_DIR_INODE *)get_child(parent_inode, name);
            if (existing_inode == NULL) {
                // If the parent inode doesn't exist, we need to create it
                uint64_t metadata_id = parent_entries[j]->id;
                parent_inode = init_dir_inode(metadata_id, current_index_context, parent_inode, context);
                if (parent_inode == NULL) {
                    return WOWS_ERROR_UNKNOWN;
                }
            } else {
                // Strangely, got the wrong inode type
                // This means there are some id collision between file and dir
                if (existing_inode->type != WOWS_INODE_TYPE_DIR) {
                    return WOWS_ERROR_ID_COLLISION_FILE_DIR;
                }
                // Otherwise we just go through it
                parent_inode = existing_inode;
            }
        }
        uint64_t metadata_id = mentry->id;
        WOWS_FILE_INODE *file_inode = init_file_inode(metadata_id, current_index_context, parent_inode, context);
        if (file_inode == NULL) {
            return WOWS_ERROR_UNKNOWN;
        }
        free(parent_entries);
    }
    return 0;
}

int wows_tree(WOWS_CONTEXT *context) {
    print_inode_tree(context->root, 0);
    return 0;
}

// Check that start and end of section is withing the boundary of the mmaped file
bool checkOutOfIndex(char *start, char *end, WOWS_INDEX *index) {
    if ((index->start_address > start) || (index->end_address < start)) {
        return true;
    }
    if ((index->start_address > end) || (index->end_address < end)) {
        return true;
    }
    return false;
}

// Free the whole context, including un-mmaping the index files;
int wows_free_context(WOWS_CONTEXT *context) {
    for (int i = 0; i < context->index_count; i++) {
        WOWS_INDEX *index = context->indexes[i];
        munmap(index->start_address, index->length);
        free(index);
    }
    return wows_free_context_no_munmap(context);
    return 0;
}

// Free without the un-mmapping (in case the index is loaded in another fashion)
int wows_free_context_no_munmap(WOWS_CONTEXT *context) {
    free_inode_tree(context->root);
    free(context->indexes);
    hashmap_free(context->metadata_map);
    hashmap_free(context->file_map);
    free(context);
    return 0;
}

// Helper functions to free the inode tree
bool iter_inode_free(const void *item, void *udata) {
    free_inode_tree(*(WOWS_BASE_INODE **)item);
    return true;
}

int free_inode_tree(WOWS_BASE_INODE *inode) {
    switch (inode->type) {
    case WOWS_INODE_TYPE_FILE:
        free(inode);
        break;
    case WOWS_INODE_TYPE_DIR:
        hashmap_scan(((WOWS_DIR_INODE *)inode)->children_inodes, iter_inode_free, NULL);
        hashmap_free(((WOWS_DIR_INODE *)inode)->children_inodes);
        free(inode);
        break;
    default:
        return WOWS_ERROR_UNKNOWN;
    }
    return 0;
}
