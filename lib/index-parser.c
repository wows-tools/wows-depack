#include <stddef.h>
#include <stdio.h>
#include "wows-depack.h"
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
    printf("* offset_idx_data_section:   0x%lx\n",
           header->offset_idx_data_section);
    printf("* offset_idx_footer_section: 0x%lx\n",
           header->offset_idx_footer_section);
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

int metadata_compare(const void *a, const void *b, void *udata) {
    const WOWS_INDEX_METADATA_ENTRY *ma = *(WOWS_INDEX_METADATA_ENTRY **)a;
    const WOWS_INDEX_METADATA_ENTRY *mb = *(WOWS_INDEX_METADATA_ENTRY **)b;
    return ma->id > mb->id;
}

uint64_t metadata_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const WOWS_INDEX_METADATA_ENTRY *meta = *(WOWS_INDEX_METADATA_ENTRY **)item;
    return meta->id;
}

char *get_metadata_filename(WOWS_INDEX_METADATA_ENTRY *entry) {
    char *filename = (char *)entry;
    filename += entry->offset_idx_file_name;
    return filename;
}

int print_debug_raw(WOWS_INDEX_HEADER *header,
                    WOWS_INDEX_METADATA_ENTRY *metadatas,
                    WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry,
                    WOWS_INDEX_FOOTER *footer) {
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
        printf("* filename:                  %.*s\n",
               (int)entry->file_name_size, get_metadata_filename(entry));
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
    char *pkg_filename = (char *)footer;
    pkg_filename += sizeof(WOWS_INDEX_FOOTER);

    printf("Index Footer Content:\n");
    print_footer(footer);
    printf("* pkg filename:              %.*s\n",
           (int)footer->pkg_file_name_size, pkg_filename);
    printf("\n");
    return 0;
}

int print_debug_files(WOWS_INDEX_HEADER *header,
                      WOWS_INDEX_METADATA_ENTRY *metadatas,
                      WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry,
                      WOWS_INDEX_FOOTER *footer, struct hashmap *map) {
    int i;
    // Print the actual files
    for (i = 0; i < header->file_count; i++) {
        WOWS_INDEX_DATA_FILE_ENTRY *fentry = &data_file_entry[i];
        WOWS_INDEX_METADATA_ENTRY *mentry_search =
            &(WOWS_INDEX_METADATA_ENTRY){.id = fentry->metadata_id};
        WOWS_INDEX_METADATA_ENTRY *mentry =
            *(WOWS_INDEX_METADATA_ENTRY **)hashmap_get(map, &mentry_search);
        printf("File entry [%d]:\n", i);
        print_data_file_entry(fentry);
        print_metadata_entry(mentry);
        printf("* filename:                  %.*s\n",
               (int)mentry->file_name_size, get_metadata_filename(mentry));
        WOWS_INDEX_METADATA_ENTRY *m_parent_entry_search =
            &(WOWS_INDEX_METADATA_ENTRY){.id = mentry->parent_id};
        WOWS_INDEX_METADATA_ENTRY **mparent_entry =
            (WOWS_INDEX_METADATA_ENTRY **)hashmap_get(map,
                                                      &m_parent_entry_search);
        int level = 1;
        while (mparent_entry != NULL) {
            printf("parent [%d]:\n", level);
            print_metadata_entry(*mparent_entry);
            printf("* filename:                  %.*s\n",
                   (int)mentry->file_name_size,
                   get_metadata_filename(*mparent_entry));
            mentry_search =
                &(WOWS_INDEX_METADATA_ENTRY){.id = (*mparent_entry)->parent_id};
            mparent_entry = NULL;
            mparent_entry =
                (WOWS_INDEX_METADATA_ENTRY **)hashmap_get(map, &mentry_search);
            level++;
        }
        printf("\n");
    }
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

    // Index all the metadata entries into an hmap
    struct hashmap *map =
        hashmap_new(sizeof(WOWS_INDEX_METADATA_ENTRY *), header->file_dir_count,
                    0, 0, metadata_hash, metadata_compare, NULL, NULL);
    for (i = 0; i < header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *entry = &metadatas[i];
        hashmap_set(map, &entry);
    }

    if (context->debug_level & DEBUG_RAW_RECORD) {
        print_debug_raw(header, metadatas, data_file_entry, footer);
    }

    if (context->debug_level & DEBUG_FILE_LISTING) {
        print_debug_files(header, metadatas, data_file_entry, footer, map);
    }

    hashmap_free(map);

    return 0;
}
