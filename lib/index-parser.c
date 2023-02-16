#include <stddef.h>
#include <stdio.h>
#include "wows-depack.h"

int print_header(WOWS_INDEX_HEADER *header) {
    printf("Index Header Content:\n");
    printf("* magic:                     %.4s\n", (char *)&header->magic);
    printf("* unknown_1:                 0x%x\n", header->unknown_1);
    printf("* id:                        0x%x\n", header->id);
    printf("* unknown_2:                 0x%x\n", header->unknown_2);
    printf("* file_plus_dir_count:       %u\n", header->file_plus_dir_count);
    printf("* file_count:                %u\n", header->file_count);
    printf("* unknown_3:                 %lu\n", header->unknown_3);
    printf("* header_size:               %lu\n", header->header_size);
    printf("* offset_idx_data_section:   0x%lx\n",
           header->offset_idx_data_section);
    printf("* offset_idx_footer_section: 0x%lx\n",
           header->offset_idx_footer_section);
    return 0;
}

int print_metadata_entry(WOWS_INDEX_METADATA_ENTRY *entry, int index) {
    printf("Metadata entry %d:\n", index);
    printf("* file_type:                 %lu\n", entry->file_type_1);
    printf("* offset_idx_file_name:      %lx\n", entry->offset_idx_file_name);
    printf("* unknown_4:                 %lx\n", entry->unknown_4);
    printf("* file_type_2:               %lu\n", entry->file_type_2);
    return 0;
}

int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context) {
    // TODO overflow, check size

    // Get the header section
    WOWS_INDEX_HEADER *header = (WOWS_INDEX_HEADER *)contents;
    if (context->debug) {
        print_header(header);
    }

    // TODO overflow, check size

    // Recover the start of the metadata array
    WOWS_INDEX_METADATA_ENTRY *metadatas;
    metadatas =
        (WOWS_INDEX_METADATA_ENTRY *)contents + sizeof(WOWS_INDEX_HEADER);

    if (context->debug) {
        print_metadata_entry(metadatas, 0);
    }
    return 0;
}
