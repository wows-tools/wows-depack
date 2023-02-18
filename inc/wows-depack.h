#include <stdint.h>
#include <stdbool.h>

#pragma pack(1)

#define DEBUG_RAW_RECORD (1 << 0)
#define DEBUG_FILE_LISTING (1 << 1)

// INDEX file header
typedef struct {
    char magic[4];
    uint32_t unknown_1;
    uint32_t id;
    uint32_t unknown_2;
    uint32_t file_dir_count;
    uint32_t file_count;
    uint64_t unknown_3;
    uint64_t header_size;
    uint64_t offset_idx_data_section;
    uint64_t offset_idx_footer_section;
} WOWS_INDEX_HEADER;

#define MAGIC_SECTION_OFFSET sizeof(uint32_t) * 4

// INDEX file metadata entry
typedef struct {
    uint64_t file_name_size;
    uint64_t offset_idx_file_name;
    uint64_t id;
    uint64_t parent_id;
} WOWS_INDEX_METADATA_ENTRY;

// INDEX file pkg data pointer entry
typedef struct {
    uint64_t metadata_id;
    uint64_t footer_id;
    uint64_t offset_pkg_data;
    uint32_t type_1;
    uint32_t type_2;
    uint32_t size_pkg_data;
    uint64_t id_pkg_data;
    uint32_t padding;
} WOWS_INDEX_DATA_FILE_ENTRY;

// INDEX file footer
typedef struct {
    uint64_t pkg_file_name_size;
    uint64_t unknown_7;
    uint64_t id;
} WOWS_INDEX_FOOTER;

// PKG file ID + padding
typedef struct {
    uint32_t padding_1;
    uint64_t id;
    uint32_t padding_2;
} WOWS_PKG_ID_ENTRY;

// Context for the parsing/file extraction
typedef struct {
    uint8_t debug_level;
} WOWS_CONTEXT;

int wows_inflate(FILE *source, FILE *dest, long *read);
int wows_inflate(FILE *source, FILE *dest, long *read);
void wows_zerr(int ret);
int wows_is_dir(const char *path);
int wows_inflate_all(FILE *in, char *outdir);

int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context);
