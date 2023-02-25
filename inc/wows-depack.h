/**
 * @file wows-depack.h
 *
 * @brief Header file for the wows-depack library.
 */

#include <stdint.h>
#include <stdbool.h>

/* Error codes */
#define WOWS_ERROR_CORRUPTED_FILE 1             /**< A corrupted WoWs resource file. */
#define WOWS_ERROR_BAD_MAGIC 2                  /**< A bad magic number in a WoWs resource file. */
#define WOWS_ERROR_MISSING_METADATA_ENTRY 3     /**< A missing metadata entry in a WoWs resource file. */
#define WOWS_ERROR_MAX_LEVEL_REACHED 4          /**< The maximum directory nesting level has been reached. */
#define WOWS_ERROR_NON_ZERO_TERMINATED_STRING 5 /**< A non-zero-terminated string in a WoWs resource file. */
#define WOWS_ERROR_PATH_TOO_LONG 6              /**< A path is too long to be processed. */
#define WOWS_ERROR_UNKNOWN 7                    /**< An unknown error occurred. */
#define WOWS_ERROR_ID_COLLISION_FILE_DIR 8      /**< A file and directory identifier collision. */
#define WOWS_ERROR_FILE_OPEN_FAILURE 9          /**< A failure to open a file. */
#define WOWS_ERROR_DECOMPOSE_PATH 10            /**< An error while decomposing a path. */

/**
 * @brief Debug levels.
 *
 * These are the available debug levels for the library.
 */
#define NO_DEBUG 0                  /**< No debug output. */
#define DEBUG_RAW_RECORD (1 << 0)   /**< Debug output for raw records. */
#define DEBUG_FILE_LISTING (1 << 1) /**< Debug output for file listings. */
/* ---------- */

/**
 * @brief WoWs resource extractor context.
 *
 * This struct is used to hold the context for the WoWs extractor.
 */
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

/* Context init/destruction */

/**
 * @brief Initializes a WoWs resource extractor context.
 *
 * This function creates and initializes a new WoW Stats Parser context with the given
 * debug level.
 *
 * @param debug_level The debug level for the parser.
 *
 * @return A pointer to the newly created context, or NULL if an error occurred.
 */
WOWS_CONTEXT *wows_init_context(uint8_t debug_level);

/**
 * @brief Frees a WoWs resource extractor context.
 *
 * This function frees the memory allocated for a WoWs resource extractor context.
 *
 * @param context A pointer to the context to free.
 *
 * @return 0 on success, or a negative value if an error occurred.
 */
int wows_free_context(WOWS_CONTEXT *context);

/**
 * @brief Frees a WoW Stats Parser context without unmapping the memory.
 *
 * This function frees the memory allocated for a WoWs resource extractor context, but does not
 * unmap the memory or close files from the process address space. This function should only be used
 * if the raw `wows_parse_index_buffer` function was used.
 *
 * @param context A pointer to the context to free.
 *
 * @return 0 on success, or a negative value if an error occurred.
 */
int wows_free_context_no_munmap(WOWS_CONTEXT *context);
/* ---------- */

/* Index parsing functions */

/**
 * @brief Parses a single WoWs resource index file.
 *
 * This function parses a single WoWs resource index file with the given path and updates
 * the given parser context with the parsed data.
 *
 * @param index_file_path The path to the WoWs resource index file to parse.
 * @param context A pointer to the parser context to update.
 *
 * @return 0 on success, or a negative value if an error occurred.
 */
int wows_parse_index_file(const char *index_file_path, WOWS_CONTEXT *context);

/**
 * @brief Parses all files in a given directory.
 *
 * This function parses all WoWs resource index files in a given directory with the
 * given path and updates the given parser context with the parsed data.
 *
 * @param index_dir_path The path to the directory containing the WoWs resource index files to parse.
 * @param context A pointer to the parser context to update.
 *
 * @return 0 on success, or a negative value if an error occurred.
 */
int wows_parse_index_dir(const char *index_dir_path, WOWS_CONTEXT *context);

/**
 * @brief Low level, parses a memory buffer directly.
 *
 * This function parses a WoWs resource index buffer directly from memory with the given
 * contents and length, and updates the given parser context with the parsed data.
 *
 * @param contents A pointer to the buffer containing the WoWs resource index contents to parse.
 * @param length The length of the buffer in bytes.
 * @param index_file_path The path to the WoWs resource index file.
 * @param fd The file descriptor associated with the index file.
 * @param context A pointer to the parser context to update.
 *
 * @return 0 on success, or a negative value if an error occurred.
 */
int wows_parse_index_buffer(char *contents, size_t length, const char *index_file_path, int fd, WOWS_CONTEXT *context);

/* Archive exploration functions */

/* search recursively files matching a pcre regexp in the archive tree */
// Not implemented
int wows_search(WOWS_CONTEXT *context, char *pattern, int mode, int *result_count, char ***results);

/* read the content of a directory */
// Not implemented
int wows_readdir(WOWS_CONTEXT *context, char dir_path, char **result[]);

// Not implemented
/* give the stat of a given path (mainly if it's a directory or a file, and the file size) */
int wows_stat_path(WOWS_CONTEXT *context, char path);

/* ---------- */

/* Archive extraction functions */

// Not implemented
/* open a given file */
int wows_open_file(WOWS_CONTEXT *context, char file_path, FILE *stream);

/* ---------- */

/* Debugging/output functions */

/**
 * @brief Prints the directory tree of the given WoWs resource parser context in a tree format.
 *
 * This function prints the directory tree of the given WoWs resource parser context in a tree format.
 *
 * @param context A pointer to the WoWs resource parser context to print.
 *
 * @return 0 on success, or a negative value if an error occurred.
 */
int wows_print_tree(WOWS_CONTEXT *context);

/**
 * @brief Prints the directory tree of the given WoWs resource parser context in a flat format.
 *
 * This function prints the directory tree of the given WoWs resource parser context in a flat format (each file with
 * its full path).
 *
 * For example:
 * /dir1/file2
 * /dir3/dir4/file5
 *
 *
 * @param context A pointer to the WoWs resource parser context to print.
 *
 * @return 0 on success, or a negative value if an error occurred.
 */
int wows_print_flat(WOWS_CONTEXT *context);

/**
 * @brief Converts an error code into an error message string for the given WoWs resource parser context.
 *
 * This function converts an error code into an error message string for the given WoWs resource parser context.
 *
 * @param error_code The error code to convert.
 * @param context A pointer to the WoWs resource parser context to use.
 *
 * @return A pointer to a string containing the error message.
 */
char *wows_error_string(int error_code, WOWS_CONTEXT *context);

/* ---------- */
