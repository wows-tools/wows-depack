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
    printf("* pkg_file_name_size:        %lu\n", footer->pkg_file_name_size);
    printf("* unknown_7:                 0x%lx\n", footer->unknown_7);
    printf("* unknown_6:                 0x%lx\n", footer->unknown_6);
    return 0;
}

int print_metadata_entry(WOWS_INDEX_METADATA_ENTRY *entry, int index) {
    printf("Metadata entry [%d]:\n", index);
    printf("* file_name_size:            %lu\n", entry->file_name_size);
    printf("* offset_idx_file_name:      0x%lx\n", entry->offset_idx_file_name);
    printf("* unknown_4:                 0x%lx\n", entry->unknown_4);
    printf("* file_type_2:               0x%lx\n", entry->file_type_2);
    return 0;
}

int print_data_file_entry(WOWS_INDEX_DATA_FILE_ENTRY *entry, int index) {
    printf("Data file entry [%d]:\n", index);
    printf("* unknown_5:                 0x%lx\n", entry->unknown_5);
    printf("* unknown_6:                 0x%lx\n", entry->unknown_6);
    printf("* offset_pkg_data_chunk:     0x%lx\n",
           entry->offset_pkg_data_chunk);
    printf("* type_1:                    0x%x\n", entry->type_1);
    printf("* type_2:                    0x%x\n", entry->type_2);
    printf("* size_pkg_data_chunk:       0x%x\n", entry->size_pkg_data_chunk);
    printf("* id_pkg_data_chunk:         0x%lx\n", entry->id_pkg_data_chunk);
    printf("* padding:                   0x%x\n", entry->padding);
    return 0;
}

int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context) {
    // TODO overflow, check size
    int i;

    // Get the header section
    WOWS_INDEX_HEADER *header = (WOWS_INDEX_HEADER *)contents;

    // Get the start of the metadata array
    WOWS_INDEX_METADATA_ENTRY *metadatas;
    metadatas =
        (WOWS_INDEX_METADATA_ENTRY *)(contents + sizeof(WOWS_INDEX_HEADER));

    // Get the start  pkg data pointer section
    WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry =
        (WOWS_INDEX_DATA_FILE_ENTRY *)(contents +
                                       header->offset_idx_data_section +
                                       MAGIC_SECTION_OFFSET);

    // Get the footer section
    WOWS_INDEX_FOOTER *footer =
        (WOWS_INDEX_FOOTER *)(contents + header->offset_idx_footer_section +
                              MAGIC_SECTION_OFFSET);
    char *pkg_filename = (char *)footer;
    pkg_filename += sizeof(WOWS_INDEX_FOOTER);

    // Print the header
    if (context->debug) {
        print_header(header);
        printf("\n");
    }

    // Print the metadata entries
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

    // Print the pkg file entries
    for (i = 0; i < header->file_count; i++) {
        WOWS_INDEX_DATA_FILE_ENTRY *entry = &data_file_entry[i];
        if (context->debug) {
            print_data_file_entry(entry, i);
            printf("\n");
        }
    }

    // Print the footer
    if (context->debug) {
        print_footer(footer);
        printf("* pkg filename:              %.*s\n",
               (int)footer->pkg_file_name_size, pkg_filename);
        printf("\n");
    }

    return 0;
}
