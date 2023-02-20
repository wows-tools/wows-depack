#include <stdint.h>
#include <stdbool.h>
#include "hashmap.h"

#pragma pack(1)

/* Limits */

// Maximum number of directories in a path (protection against infinite loops)
// In practice, the observed maximum depth is 7. 32 should be more than enough
#define WOWS_DIR_MAX_LEVEL 32
// Put some limits on the file name size
#define WOWS_PATH_MAX 4096

/* ---------- */

/* Errors */

#define returnOutIndex(start, end, index)                                                                              \
    if (checkOutOfIndex(start, end, index)) {                                                                          \
        return WOWS_ERROR_CORRUPTED_FILE;                                                                              \
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

#define WOWS_ROOT_INODE 0

#define WOWS_INODE_TYPE_DIR 0
#define WOWS_INODE_TYPE_FILE 1

// Base inode
typedef struct {
    uint8_t type;              // type of node (either WOWS_FILE_INODE or WOWS_DIR_INODE
    uint64_t id;               // id of the corresponding metadata item
    uint32_t index_file_index; // index of the index in WOWS_CONTEXT.indexes
    char *name;                // Name of the file/dir
} WOWS_BASE_INODE;

// Dir inode
typedef struct WOWS_DIR_INODE {
    uint8_t type;                        // always WOWS_DIR_INODE
    uint64_t id;                         // id of the corresponding metadata item
    uint32_t index_file_index;           // index of the index in WOWS_CONTEXT.indexes
    char *name;                          // Name of the file/dir
    struct WOWS_DIR_INODE *parent_inode; // parent inode (always a directory)
    struct hashmap *children_inodes;     // children inodes (directories or files)
} WOWS_DIR_INODE;

// file inode
typedef struct WOWS_FILE_INODE {
    uint8_t type;                        // always WOWS_FILE_INODE
    uint64_t id;                         // id of the corresponding metadata item
    uint32_t index_file_index;           // context index of the index file
    char *name;                          // Name of the file/dir
    struct WOWS_DIR_INODE *parent_inode; // parent inode (always a directory)
} WOWS_FILE_INODE;

// Index File structure
typedef struct WOWS_INDEX {
    WOWS_INDEX_HEADER *header;
    WOWS_INDEX_METADATA_ENTRY *metadata;
    WOWS_INDEX_DATA_FILE_ENTRY *data_file_entry;
    WOWS_INDEX_FOOTER *footer;
    char *start_address;
    char *end_address;
    size_t length;
} WOWS_INDEX;

/* ---------- */

int wows_inflate(FILE *source, FILE *dest, long *read);
void wows_zerr(int ret);
int wows_is_dir(const char *path);
int wows_inflate_all(FILE *in, char *outdir);
bool checkOutOfIndex(char *start, char *end, WOWS_INDEX *index);

int get_metadata_filename(WOWS_INDEX_METADATA_ENTRY *entry, WOWS_INDEX *index, char **out);
int get_footer_filename(WOWS_INDEX_FOOTER *footer, WOWS_INDEX *index, char **out);

int print_header(WOWS_INDEX_HEADER *header);
int print_footer(WOWS_INDEX_FOOTER *footer);
int print_metadata_entry(WOWS_INDEX_METADATA_ENTRY *entry);
int print_data_file_entry(WOWS_INDEX_DATA_FILE_ENTRY *entry);
int print_debug_files(WOWS_INDEX *index, struct hashmap *map);
int print_debug_raw(WOWS_INDEX *index);
int print_inode_tree(WOWS_BASE_INODE *inode, int level);
bool dir_inode_print(const void *item, void *udata);
WOWS_BASE_INODE *get_child(WOWS_DIR_INODE *inode, char *name);
