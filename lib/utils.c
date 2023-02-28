#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pcre.h>
#include <zlib.h>
#include <stdbool.h>
#include <string.h>

#include "wows-depack.h"
#include "wows-depack-private.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

/**
 * Compiles a PCRE regular expression pattern and returns a pointer to the compiled expression.
 *
 * @param pattern The regular expression pattern to compile.
 * @return A pointer to the compiled regular expression object, or NULL if an error occurred during compilation.
 */
pcre *compile_regex(const char *pattern) {
    int erroffset;
    const char *error;
    pcre *re;

    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (!re) {
        return NULL;
    }

    return re;
}

/**
 * Matches a PCRE regular expression against a subject string and prints the first match found.
 *
 * @param re A pointer to the compiled regular expression object.
 * @param subject The subject string to match against.
 * @return 0 if a match is found, or 1 if no match is found or an error occurs during matching.
 */
bool match_regex(pcre *re, const char *subject) {
    int rc;
    int ovector[3];

    rc = pcre_exec(re, NULL, subject, strlen(subject), 0, 0, ovector, 3);
    if (rc < 0) {
        switch (rc) {
        case PCRE_ERROR_NOMATCH:
            break;
        default:
            break;
        }
        return false;
    }

    return true;
}

int free_regex(pcre *re) {
    pcre_free(re);
    return 0;
}

/**
 * Decomposes a file path into its component parts.
 *
 * @param path the file path to decompose
 * @param out_dir_count a pointer to an integer that will receive the number of parent directories in the path
 * @param out_dirs a pointer to a char array that will receive the parent directory names
 * @param out_file a pointer to a char pointer that will receive the file name (or the entire path if no directories are
 * present)
 * @return 0 on success, or a non-zero error code on failure
 */
int decompose_path(const char *path, int *out_dir_count, char ***out_dirs, char **out_file) {
    const char DIR_SEP = '/';
    const int MAX_DIRS = 10;

    // Find last directory separator character
    const char *sep = strrchr(path, DIR_SEP);
    if (!sep) {
        // No directory separator found
        char *file = calloc(sizeof(char), (strlen(path) + 1));
        memcpy(file, path, strlen(path));
        *out_file = file;
        *out_dirs = NULL;
        *out_dir_count = 0;
        return 0;
    }

    // Count parent directories
    int count = 0;
    const char *cur_dir = path;
    const char *next_dir = path;
    while (count < MAX_DIRS && (sep = strchr(cur_dir, DIR_SEP))) {
        next_dir = sep + 1;
        // Ignore initial '/' and '//[////]'
        if (cur_dir + 1 != next_dir) {
            count++;
        }
        cur_dir = next_dir;
    }

    // Allocate array of directory strings
    char **dirs = malloc(sizeof(char *) * count);
    if (!dirs) {
        return WOWS_ERROR_DECOMPOSE_PATH;
    }

    // Copy directory paths to array in reverse order
    cur_dir = path;
    int i = 0;
    while (i < count) {
        sep = strchr(cur_dir, DIR_SEP);
        next_dir = sep + 1;
        if (cur_dir + 1 != next_dir) {
            size_t dir_len = sep - cur_dir;
            dirs[i] = malloc(sizeof(char) * (dir_len + 1));
            if (!dirs[i]) {
                return WOWS_ERROR_DECOMPOSE_PATH;
            }
            strncpy(dirs[i], cur_dir, dir_len);
            dirs[i][dir_len] = '\0';
            i++;
        }
        cur_dir = next_dir;
    }

    char *file;
    if (strlen(cur_dir) != 0) {
        file = calloc(sizeof(char), (strlen(cur_dir) + 1));
        memcpy(file, cur_dir, strlen(cur_dir));
    } else {
        file = NULL;
    }
    memcpy(file, cur_dir, strlen(cur_dir));

    *out_dirs = dirs;
    *out_dir_count = count;
    *out_file = file;
    return 0;
}

char *join_path(char **parent_entries, int depth, char *name) {
    // Calculate the total length of the combined path
    size_t combined_len = 0;
    for (int i = depth - 1; i > -1; i--) {
        combined_len += strlen(parent_entries[i]) + 1; // +1 for the slash
    }

    // Allocate memory for the combined path string
    char *combined_path = malloc(combined_len + strlen(name) + 1); // +1 for the null terminator
    if (!combined_path) {
        return NULL;
    }

    // Copy each path component into the combined path string with a slash in between
    size_t offset = 0;
    for (int i = depth - 1; i > -1; i--) {
        size_t len = strlen(parent_entries[i]);
        memcpy(combined_path + offset, parent_entries[i], len);
        offset += len;
        combined_path[offset++] = '/';
    }
    memcpy(combined_path + offset, name, strlen(name));
    combined_path[combined_len + strlen(name)] = '\0';

    return combined_path;
}
