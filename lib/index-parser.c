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

int print_footer(WOWS_INDEX_FOOTER *footer) {
    printf("Index Footer Content:\n");
    printf("* size_pkg_file_name:        %lu\n", footer->size_pkg_file_name);
    printf("* unknown_7:                 0x%lx\n", footer->unknown_7);
    printf("* unknown_6:                 0x%lx\n", footer->unknown_6);
    return 0;
}

int print_metadata_entry(WOWS_INDEX_METADATA_ENTRY *entry, int index) {
    printf("Metadata entry %d:\n", index);
    printf("* file_name_size:            %lu\n", entry->file_name_size);
    printf("* offset_idx_file_name:      0x%lx\n", entry->offset_idx_file_name);
    printf("* unknown_4:                 0x%lx\n", entry->unknown_4);
    printf("* file_type_2:               0x%lx\n", entry->file_type_2);
    return 0;
}

int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context) {
    // TODO overflow, check size
    int i;

    // Get the header section
    WOWS_INDEX_HEADER *header = (WOWS_INDEX_HEADER *)contents;
    if (context->debug) {
        print_header(header);
        printf("\n");
    }

    // Get the footer section
    WOWS_INDEX_FOOTER *footer =
        (WOWS_INDEX_FOOTER *)(contents + header->offset_idx_footer_section +
                              MAGIC_SECTION_OFFSET);
    char *pkg_filename = (char *)footer;
    pkg_filename += sizeof(WOWS_INDEX_FOOTER);
    if (context->debug) {
        print_footer(footer);
        printf("* pkg filename:              %.*s\n",
               (int)footer->size_pkg_file_name, pkg_filename);
        printf("\n");
    }

    // TODO overflow, check size

    // Recover the start of the metadata array
    WOWS_INDEX_METADATA_ENTRY *metadatas;
    metadatas =
        (WOWS_INDEX_METADATA_ENTRY *)(contents + sizeof(WOWS_INDEX_HEADER));

    for (i = 0; i < header->file_plus_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *entry = &metadatas[i];
        char *filename = (char *)entry;
        filename += entry->offset_idx_file_name;
        if (context->debug) {
            print_metadata_entry(entry, i);
            printf("* filename:                  %.*s\n",
                   (int)entry->file_name_size, filename);
            printf("\n");
        }
    }

    return 0;
}
