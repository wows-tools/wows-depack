#include <stdint.h>
#include <stdbool.h>
#include "hashmap.h"

#pragma pack(1)

/* Limits */

#define WOWS_DIR_MAX_LEVEL                                                     \
    128 // Maximum number of directory in a path (protection against infinite
        // loops)

/* ---------- */

/* Errors */

#define WOWS_ERROR_CORRUPTED_FILE 1
#define WOWS_ERROR_BAD_MAGIC 2
#define WOWS_ERROR_MISSING_METADATA_ENTRY 3
#define WOWS_ERROR_MAX_LEVEL_REACHED 4

#define returnOutIndex(start, end, index)                                      \
    if (checkOutOfIndex(start, end, index)) {                                  \
        return WOWS_ERROR_CORRUPTED_FILE;                                      \
    }

/* ---------- */

/* Structures describing the file format */

#define MAGIC_SECTION_OFFSET sizeof(uint32_t) * 4

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

/* ---------- */

/* Simple Inode representation of the tree */

#define FILE_INODE (1 << 0)
#define DIR_INODE (1 << 1)

// Base inode
typedef struct {
    uint8_t type;
    uint64_t id;
} WOWS_BASE_INODE;

// Dir inode
typedef struct WOWS_DIR_INODE {
    uint8_t type;
    uint64_t id;
    struct WOWS_DIR_INODE *parent_inode;
    struct WOWS_DIR_INODE *children_inodes[];
} WOWS_DIR_INODE;

// file inode
typedef struct {
    uint8_t type;
    uint64_t id;
    struct WOWS_DIR_INODE *parent_inode;
} WOWS_FILE_INODE;

// Index File structure
typedef struct {
    WOWS_INDEX_HEADER *header;
    WOWS_INDEX_METADATA_ENTRY *metadata;
    WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry;
    WOWS_INDEX_FOOTER *footer;
    char *start_address;
    char *end_address;
} WOWS_INDEX;

/* ---------- */

/* Parsing/Extract Context */

#define DEBUG_RAW_RECORD (1 << 0)
#define DEBUG_FILE_LISTING (1 << 1)

// Context for the parsing/file extraction
typedef struct {
    uint8_t debug_level;          // Debug level for logging
    WOWS_DIR_INODE *root;         // Root Inode
    struct hashmap *metadata_map; // Global Metadata hashmap
    WOWS_DIR_INODE *current_dir;  // Current directory
    WOWS_INDEX *indexes;  // Array of structures representing each index file
    uint32_t index_count; // Size of the array
} WOWS_CONTEXT;

/* ---------- */

WOWS_CONTEXT *wows_init_context(uint8_t debug_level);
int wows_free_context(WOWS_CONTEXT *);

int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context);

// TODO remove from the header
int wows_inflate(FILE *source, FILE *dest, long *read);
void wows_zerr(int ret);
int wows_is_dir(const char *path);
int wows_inflate_all(FILE *in, char *outdir);
bool checkOutOfIndex(char *start, char *end, WOWS_INDEX *index);
