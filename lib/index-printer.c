#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wows-depack.h"
#include "wows-depack-private.h"
#include "hashmap.h"

int print_header(WOWS_INDEX_HEADER *header) {
    printf("* magic:                     %.4s\n", (char *)&header->magic);
    printf("* unknown_1:                 0x%x\n", header->unknown_1);
    printf("* id:                        0x%x\n", header->id);
    printf("* unknown_2:                 0x%x\n", header->unknown_2);
    printf("* file_dir_count:            %u\n", header->file_dir_count);
    printf("* file_count:                %u\n", header->file_count);
    printf("* unknown_3:                 %lu\n", header->unknown_3);
    printf("* header_size:               %lu\n", header->header_size);
    printf("* offset_idx_data_section:   0x%lx\n", header->offset_idx_data_section);
    printf("* offset_idx_footer_section: 0x%lx\n", header->offset_idx_footer_section);
    return 0;
}

int print_footer(WOWS_INDEX_FOOTER *footer) {
    printf("* pkg_file_name_size:        %lu\n", footer->pkg_file_name_size);
    printf("* unknown_7:                 0x%lx\n", footer->unknown_7);
    printf("* id:                        0x%lx\n", footer->id);
    return 0;
}

int print_metadata_entry(WOWS_INDEX_METADATA_ENTRY *entry) {
    printf("* file_name_size:            %lu\n", entry->file_name_size);
    printf("* offset_idx_file_name:      0x%lx\n", entry->offset_idx_file_name);
    printf("* id:                        0x%lx\n", entry->id);
    printf("* parent_id:                 0x%lx\n", entry->parent_id);
    return 0;
}

int print_data_file_entry(WOWS_INDEX_DATA_FILE_ENTRY *entry) {
    printf("* metadata_id:               0x%lx\n", entry->metadata_id);
    printf("* footer_id:                 0x%lx\n", entry->footer_id);
    printf("* offset_pkg_data:           0x%lx\n", entry->offset_pkg_data);
    printf("* type_1:                    0x%x\n", entry->type_1);
    printf("* type_2:                    0x%x\n", entry->type_2);
    printf("* size_pkg_data:             0x%x\n", entry->size_pkg_data);
    printf("* id_pkg_data:               0x%lx\n", entry->id_pkg_data);
    printf("* padding:                   0x%x\n", entry->padding);
    return 0;
}

int print_debug_raw(WOWS_INDEX *index) {
    WOWS_INDEX_HEADER *header = index->header;
    WOWS_INDEX_METADATA_ENTRY *metadatas = index->metadata;
    WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry = index->data_file_entry;
    WOWS_INDEX_FOOTER *footer = index->footer;
    int i;
    // Print the header
    printf("Index Header Content:\n");
    print_header(header);
    printf("\n");

    // Print the metadata entries
    for (i = 0; i < header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *entry = &metadatas[i];
        printf("Metadata entry [%d]:\n", i);
        print_metadata_entry(entry);
        char *filename;
        int ret = get_metadata_filename(entry, index, &filename);
        if (ret == 0) {
            printf("* filename:                  %.*s\n", (int)entry->file_name_size, filename);
        } else {
            printf("* pkg filename:              Error %d\n", ret);
        }
        printf("\n");
    }

    // Print the pkg file entries
    for (i = 0; i < header->file_count; i++) {
        WOWS_INDEX_DATA_FILE_ENTRY *entry = &data_file_entry[i];
        printf("Data file entry [%d]:\n", i);
        print_data_file_entry(entry);
        printf("\n");
    }

    // Print the footer

    printf("Index Footer Content:\n");
    print_footer(footer);
    char *pkg_filename;
    int ret = get_footer_filename(footer, index, &pkg_filename);
    if (ret == 0) {
        printf("* pkg filename:              %.*s\n", (int)footer->pkg_file_name_size, pkg_filename);
    } else {
        printf("* pkg filename:              Error %d\n", ret);
    }
    printf("\n");
    return 0;
}

int print_debug_files(WOWS_INDEX *index, struct hashmap *map) {
    WOWS_INDEX_HEADER *header = index->header;
    WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry = index->data_file_entry;
    int i;
    // Print the actual files
    for (i = 0; i < header->file_count; i++) {
        WOWS_INDEX_DATA_FILE_ENTRY *fentry = &data_file_entry[i];
        WOWS_INDEX_METADATA_ENTRY *mentry_search = &(WOWS_INDEX_METADATA_ENTRY){.id = fentry->metadata_id};
        void *res = hashmap_get(map, &mentry_search);
        if (res == NULL) {
            return WOWS_ERROR_MISSING_METADATA_ENTRY;
        }
        WOWS_INDEX_METADATA_ENTRY *mentry = *(WOWS_INDEX_METADATA_ENTRY **)res;

        printf("File entry [%d]:\n", i);
        print_data_file_entry(fentry);
        print_metadata_entry(mentry);
        char *filename;
        int ret = get_metadata_filename(mentry, index, &filename);

        if (ret == 0) {
            printf("* filename:                  %.*s\n", (int)mentry->file_name_size, filename);
        } else {
            printf("* pkg filename:              Error %d\n", ret);
        }
        WOWS_INDEX_METADATA_ENTRY *m_parent_entry_search = &(WOWS_INDEX_METADATA_ENTRY){.id = mentry->parent_id};
        WOWS_INDEX_METADATA_ENTRY **mparent_entry =
            (WOWS_INDEX_METADATA_ENTRY **)hashmap_get(map, &m_parent_entry_search);
        int level = 1;
        while (mparent_entry != NULL && level < WOWS_DIR_MAX_LEVEL) {
            printf("parent [%d]:\n", level);
            print_metadata_entry(*mparent_entry);
            char *filename;
            int ret = get_metadata_filename(*mparent_entry, index, &filename);

            if (ret == 0) {
                printf("* filename:                  %.*s\n", (int)mentry->file_name_size, filename);
            } else {
                printf("* pkg filename:              Error %d\n", ret);
            }

            mentry_search = &(WOWS_INDEX_METADATA_ENTRY){.id = (*mparent_entry)->parent_id};
            mparent_entry = NULL;
            mparent_entry = (WOWS_INDEX_METADATA_ENTRY **)hashmap_get(map, &mentry_search);
            level++;
        }
        printf("\n");
    }
    return 0;
}
