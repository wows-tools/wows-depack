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
#define WOWS_ERROR_ID_COLLISION_FILE_DIR 8
#define WOWS_ERROR_FILE_OPEN_FAILURE 9

/* Debug levels */
#define NO_DEBUG 0
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
    char *err_msg;        // Last error message
} WOWS_CONTEXT;

/* ---------- */

/* Parser context init/free */
WOWS_CONTEXT *wows_init_context(uint8_t debug_level);

/* Free function */
int wows_free_context(WOWS_CONTEXT *);

/* Free function in case you use the raw wows_parse_index_buffer */
int wows_free_context_no_munmap(WOWS_CONTEXT *context);
/* ---------- */

/* parse one file */
int wows_parse_index(char *index_file_path, WOWS_CONTEXT *context);

/* low level, parse a memory buffer directly */
int wows_parse_index_buffer(char *contents, size_t length, char *index_file_path, WOWS_CONTEXT *context);

// Not implemented
/* search recursively files matching a pcre regexp in the archive tree */
int wows_search_files(WOWS_CONTEXT *context, char *regexp, char **results[]);

// Not implemented
/* search recursively files matching a pcre regexp in the archive tree */
int wows_readdir(WOWS_CONTEXT *context, char dir_path, char **result[]);

// Not implemented
/* open a given file */
int wows_open_file(WOWS_CONTEXT *context, char file_path, FILE *stream);


// Not implemented
/* give the stat of a given path (mainly if it's a directory or a file, and the file size) */
int wows_stat_path(WOWS_CONTEXT *context, char path);

/* print the directory tree in a tree format*/
int wows_print_tree(WOWS_CONTEXT *context);

/* print the directory tree in a tree format*/
int wows_print_flat(WOWS_CONTEXT *context);

/* convert error code into error messages */
char *wows_error_string(int error_code, WOWS_CONTEXT *context);
