#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

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

    // Remove remaining '/' from the file name
    int limit = 0;
    while (limit < MAX_DIRS && (sep = strchr(cur_dir, DIR_SEP))) {
        next_dir = sep + 1;
        cur_dir = next_dir;
        limit++;
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

int wows_get_latest_idx_dir(char *wows_base_dir, char **idx_dir) {
    char *bin_dir = calloc(sizeof(char), strlen(wows_base_dir) + 5);
    sprintf(bin_dir, "%s/bin", wows_base_dir);
    DIR *dir_ptr = opendir(bin_dir); // open the wows bin directory
    struct stat info;
    char *out = NULL;
    if (dir_ptr == NULL) { // handle error
        free(bin_dir);
        return WOWS_ERROR_FILE_OPEN_FAILURE;
    }

    struct dirent *entry;  // pointer to directory entry
    int current_build = 0; // Highest build number we encountered so far

    while ((entry = readdir(dir_ptr)) != NULL) { // read directory entries
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", bin_dir, entry->d_name);

        // Ignore errors
        if (stat(full_path, &info) != 0) {
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            int build = atoi(entry->d_name);
            if (current_build < build) {
                free(out);
                out = calloc(sizeof(char), strlen(full_path) + 6);
                sprintf(out, "%s/idx/", full_path);
                current_build = build;
            }
        }
    }
    free(bin_dir);
    if (out == NULL) {
        return WOWS_ERROR_FILE_OPEN_FAILURE;
    }

    if ((stat(out, &info) != 0) && (!S_ISDIR(info.st_mode))) {
        free(out);
        out = NULL;
        return WOWS_ERROR_FILE_OPEN_FAILURE;
    }

    *idx_dir = out;

    closedir(dir_ptr); // close the directory

    return 0;
}
