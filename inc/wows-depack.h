#include <stdint.h>
#include <stdbool.h>

/* Errors */

#define WOWS_ERROR_CORRUPTED_FILE 1
#define WOWS_ERROR_BAD_MAGIC 2
#define WOWS_ERROR_MISSING_METADATA_ENTRY 3
#define WOWS_ERROR_MAX_LEVEL_REACHED 4
#define WOWS_ERROR_NON_ZERO_TERMINATED_STRING 5
#define WOWS_ERROR_PATH_TOO_LONG 6
#define WOWS_ERROR_UNKNOWN 7

/* Debug levels */
#define DEBUG_RAW_RECORD (1 << 0)
#define DEBUG_FILE_LISTING (1 << 1)
/* ---------- */

// Context for the parsing/file extraction
typedef struct {
    uint8_t debug_level;  // Debug level for logging
    void *root;           // Root Inode (WOWS_DIR_INODE * root;)
    void *metadata_map;   // Global Metadata hashmap (struct hashmap *metadata_map;)
    void *file_map;       // Global File hashmap (struct hashmap *file_map;)
    void *current_dir;    // Current directory (WOWS_DIR_INODE *current_dir;)
    void **indexes;       // Array of structures representing each index file (void
                          // **indexes;)
    uint32_t index_count; // Size of the array
} WOWS_CONTEXT;

/* ---------- */

/* Parser context init/free */
WOWS_CONTEXT *wows_init_context(uint8_t debug_level);
int wows_free_context(WOWS_CONTEXT *);
/* ---------- */

/* parse one file */
int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context);
