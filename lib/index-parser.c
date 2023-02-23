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
    if (err != 0) {
        return err;
    }

    index->index_file_path = index_file_path;

    // Debugging output if necessary
    if (context->debug_level & DEBUG_RAW_RECORD) {
        print_debug_raw(index);
    }
    if (context->debug_level & DEBUG_FILE_LISTING) {
        print_debug_files(index, context->metadata_map);
    }

    char *filler;

    // Index all the metadata entries into an hmap
    for (i = 0; i < index->header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *entry = &index->metadata[i];
        // Check that we can correctly recover the file names
        int ret = get_metadata_filename(entry, index, &filler);
        if (ret != 0) {
            context->err_msg = wows_render_str("problematic metadata entry id '0x%lx'", entry->id);
            return ret;
        }
        hashmap_set(context->metadata_map, &entry);
    }
    // Do the same for all the file entries
    for (int i = 0; i < index->header->file_count; i++) {
        WOWS_INDEX_DATA_FILE_ENTRY *fentry = &index->data_file_entry[i];
        hashmap_set(context->file_map, &fentry);
    }

    // Check that we can correctly recover the file names
    int ret = get_footer_filename(index->footer, index, &filler);
    if (ret != 0) {
        context->err_msg = wows_render_str("problematic footer section");
        return ret;
    }

    uint32_t current_index_context = add_index_context(context, index);

    err = build_inode_tree(index, current_index_context, context);
    return ret;
}

// Index Dumper function
int wows_dump_index_to_file(WOWS_INDEX *index, const char *filename) {
    // TODO recompute the offsets in the metadata and header
    FILE *f = fopen(filename, "wb");
    if (!f) {
        return WOWS_ERROR_FILE_OPEN_FAILURE;
    }

    // Write the header
    fwrite(index->header, sizeof(WOWS_INDEX_HEADER), 1, f);

    // Write the metadata entries
    for (size_t i = 0; i < index->header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *metadata_entry = &index->metadata[i];
        fwrite(metadata_entry, sizeof(WOWS_INDEX_METADATA_ENTRY), 1, f);
    }

    // Write the file name strings
    for (size_t i = 0; i < index->header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *metadata_entry = &index->metadata[i];
        char *file_name;
        get_metadata_filename(metadata_entry, index, &file_name);
        fwrite(file_name, metadata_entry->file_name_size, 1, f);
    }

    // Write the data file entries
    for (size_t i = 0; i < index->header->file_count; i++) {
        WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry = &index->data_file_entry[i];
        fwrite(data_file_entry, sizeof(WOWS_INDEX_DATA_FILE_ENTRY), 1, f);
    }

    // Write the footer
    fwrite(index->footer, sizeof(WOWS_INDEX_FOOTER), 1, f);

    // Write the pkg file name
    char *pkg_file_name = (char *)index->footer + sizeof(WOWS_INDEX_FOOTER);
    fwrite(pkg_file_name, index->footer->pkg_file_name_size, 1, f);

    fclose(f);
    return 0;
}
