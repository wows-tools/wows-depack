#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wows-depack.h"
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

// FIXME potential buffer overflow, add boundary checks
char *get_metadata_filename(WOWS_INDEX_METADATA_ENTRY *entry) {
    char *filename = (char *)entry;
    filename += entry->offset_idx_file_name;
    return filename;
}

// FIXME potential buffer overflow, add boundary checks
char *get_footer_filename(WOWS_INDEX_FOOTER *footer) {
    char *pkg_filename = (char *)footer;
    pkg_filename += sizeof(WOWS_INDEX_FOOTER);
    return pkg_filename;
}

WOWS_CONTEXT *wows_init_context(uint8_t debug_level) {
    WOWS_CONTEXT *context = calloc(sizeof(WOWS_CONTEXT), 1);
    struct hashmap *map =
        hashmap_new(sizeof(WOWS_INDEX_METADATA_ENTRY *), 0, 0, 0, metadata_hash,
                    metadata_compare, NULL, NULL);
    context->metadata_map = map;
    context->debug_level = debug_level;
    return context;
}

int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context) {
    int i;
    struct hashmap *map = context->metadata_map;

    WOWS_INDEX *index = calloc(sizeof(WOWS_INDEX), 1);

    index->start_address = contents;
    index->end_address = contents + length;

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
    metadatas =
        (WOWS_INDEX_METADATA_ENTRY *)(contents + sizeof(WOWS_INDEX_HEADER));
    index->metadata = metadatas;
    // Check metadatas section boundaries
    returnOutIndex((char *)metadatas,
                   (char *)&metadatas[header->file_dir_count], index);

    // Get the start pkg data pointer section
    WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry =
        (WOWS_INDEX_DATA_FILE_ENTRY *)(contents +
                                       header->offset_idx_data_section +
                                       MAGIC_SECTION_OFFSET);
    index->data_file_entry = data_file_entry;
    // Check data_file_entries section boundaries
    returnOutIndex((char *)data_file_entry,
                   (char *)&data_file_entry[header->file_count], index);

    // Get the footer section
    WOWS_INDEX_FOOTER *footer =
        (WOWS_INDEX_FOOTER *)(contents + header->offset_idx_footer_section +
                              MAGIC_SECTION_OFFSET);
    index->footer = footer;
    // Check footer section boundaries
    returnOutIndex((char *)footer, (char *)footer + sizeof(WOWS_INDEX_FOOTER),
                   index);

    // Index all the metadata entries into an hmap
    for (i = 0; i < header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *entry = &metadatas[i];
        hashmap_set(map, &entry);
    }

    // Debugging output if necessary
    if (context->debug_level & DEBUG_RAW_RECORD) {
        print_debug_raw(header, metadatas, data_file_entry, footer);
    }
    if (context->debug_level & DEBUG_FILE_LISTING) {
        print_debug_files(header, metadatas, data_file_entry, footer, map);
    }

    return 0;
}

bool checkOutOfIndex(char *start, char *end, WOWS_INDEX *index) {
    if ((index->start_address > start) || (index->end_address < start)) {
        return true;
    }
    if ((index->start_address > end) || (index->end_address < end)) {
        return true;
    }
    return false;
}

int wows_free_context(WOWS_CONTEXT *context) {
    // TODO
    return 0;
}
