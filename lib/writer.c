#define _POSIX_C_SOURCE 200809L
// TODO clean-up this mess
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
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

int list_files_recursive(const char *path, char ***file_paths, int *count) {
    DIR *dir;
    struct dirent *entry;
    struct stat info;

    if ((dir = opendir(path)) == NULL) {
        return WOWS_ERROR_FILE_OPEN_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        // Ignore errors
        // FIXME even if not fatal we should do something with these ones
        if (stat(full_path, &info) != 0) {
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            // Ignore "." and ".." directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            list_files_recursive(full_path, file_paths, count);
        } else if (S_ISREG(info.st_mode)) {
            // Store the file path in the array
            (*count)++;
            *file_paths = realloc(*file_paths, (*count) * sizeof(char *));
            (*file_paths)[(*count) - 1] = strdup(full_path);
        }
    }

    closedir(dir);
    return 0;
}

// Index Dumper function
int wows_dump_index_to_file(WOWS_INDEX *index, FILE *f) {
    uint64_t start_data;
    uint64_t start_footer;
    uint64_t general_offset = 0;

    // Write the header

    fwrite(index->header, sizeof(WOWS_INDEX_HEADER), 1, f);
    general_offset += sizeof(WOWS_INDEX_HEADER);

    // Write the metadata entries
    uint64_t string_start = sizeof(WOWS_INDEX_METADATA_ENTRY) * index->header->file_dir_count;
    for (size_t i = 0; i < index->header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY metadata_entry = index->metadata[i];
        metadata_entry.offset_idx_file_name = string_start;
        fwrite(&metadata_entry, sizeof(WOWS_INDEX_METADATA_ENTRY), 1, f);
        general_offset += sizeof(WOWS_INDEX_METADATA_ENTRY);
        string_start -= sizeof(WOWS_INDEX_METADATA_ENTRY);
        string_start += metadata_entry.file_name_size;
    }

    // Write the file name strings
    for (size_t i = 0; i < index->header->file_dir_count; i++) {
        WOWS_INDEX_METADATA_ENTRY *metadata_entry = &index->metadata[i];
        char *file_name;
        get_metadata_filename(metadata_entry, index, &file_name);
        fwrite(file_name, metadata_entry->file_name_size, 1, f);
        general_offset += metadata_entry->file_name_size;
    }

    start_data = general_offset;
    // Write the data file entries
    for (size_t i = 0; i < index->header->file_count; i++) {
        WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry = &index->data_file_entry[i];
        fwrite(data_file_entry, sizeof(WOWS_INDEX_DATA_FILE_ENTRY), 1, f);
        general_offset += sizeof(WOWS_INDEX_DATA_FILE_ENTRY);
    }

    // Write the footer
    start_footer = general_offset;
    fwrite(index->footer, sizeof(WOWS_INDEX_FOOTER), 1, f);

    // Write the pkg file name
    char *pkg_file_name = (char *)index->footer + sizeof(WOWS_INDEX_FOOTER);
    fwrite(pkg_file_name, index->footer->pkg_file_name_size, 1, f);

    // Re-Write the header with the correct offsets
    fseek(f, 0, SEEK_SET);
    WOWS_INDEX_HEADER cpy_header = *index->header;
    cpy_header.header_size = 40;
    cpy_header.unknown_2 = 0x40;
    cpy_header.offset_idx_data_section = start_data - MAGIC_SECTION_OFFSET;
    cpy_header.offset_idx_footer_section = start_footer - MAGIC_SECTION_OFFSET;
    fwrite(&cpy_header, sizeof(WOWS_INDEX_HEADER), 1, f);

    return 0;
}
