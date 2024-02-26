#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <zlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "wows-depack.h"
#include "wows-depack-private.h"
#include "hashmap.h"

#define CHUNK_SIZE 16384 // 16KB

int get_inode(WOWS_CONTEXT *context, char *path, WOWS_BASE_INODE **out_inode) {
    int dir_count;
    char **dirs;
    char *file = NULL;
    int ret = decompose_path(path, &dir_count, &dirs, &file);
    if (ret != 0) {
        return ret;
    }
    WOWS_DIR_INODE *inode = context->root;
    for (int i = 0; i < dir_count; i++) {
        if (inode != NULL && inode->type == WOWS_INODE_TYPE_DIR) {
            WOWS_DIR_INODE *inode_search = &(WOWS_DIR_INODE){.name = dirs[i]};
            struct hashmap *map = inode->children_inodes;
            void *inode_ptr = hashmap_get(map, &inode_search);
            if (inode_ptr != NULL) {
                wows_set_error_details(context, "directory '%s' doesn't exist", dirs[i]);
                inode = *(WOWS_DIR_INODE **)inode_ptr;
            } else {
                inode = NULL;
            }
        }
        // We don't break here just to free each path items
        free(dirs[i]);
    }
    free(dirs);
    if (file == NULL && inode != NULL) {
        *out_inode = (WOWS_BASE_INODE *)inode;
        return 0;
    }

    if (inode == NULL) {
        wows_set_error_details(context, "file/dir '%s' doesn't exist", file);
        free(file);
        return WOWS_ERROR_NOT_FOUND;
    }
    if (inode->type != WOWS_INODE_TYPE_DIR) {
        wows_set_error_details(context, "'%s' is not a directory", inode->name);
        free(file);
        return WOWS_ERROR_NOT_A_DIR;
    }

    WOWS_DIR_INODE *inode_search = &(WOWS_DIR_INODE){.name = file};
    struct hashmap *map = inode->children_inodes;
    void *inode_ptr = hashmap_get(map, &inode_search);

    if (inode_ptr == NULL) {
        wows_set_error_details(context, "file/dir '%s' doesn't exist", file);
        free(file);
        return WOWS_ERROR_NOT_FOUND;
    }
    inode = *(WOWS_DIR_INODE **)inode_ptr;
    if (inode == NULL) {
        wows_set_error_details(context, "file/dir '%s' doesn't exist", file);
        free(file);
        return WOWS_ERROR_NOT_FOUND;
    }
    free(file);
    *out_inode = (WOWS_BASE_INODE *)inode;
    return 0;
}

int copy_data(FILE *in, FILE *out, long offset, size_t size) {
    char buffer[BUFSIZ];
    size_t bytes_read, bytes_written;
    long total_bytes_read = 0;
    int fseek_result;

    // seek to the specified offset in the input file
    fseek_result = fseek(in, offset, SEEK_SET);
    if (fseek_result != 0) {
        return WOWS_ERROR_UNKNOWN;
    }

    // copy data from the input file to the output file
    while (total_bytes_read < size && !feof(in) && !ferror(in)) {
        // read up to BUFSIZ bytes or the remaining number of bytes to read
        bytes_read = fread(buffer, 1, (size - total_bytes_read < BUFSIZ) ? (size - total_bytes_read) : BUFSIZ, in);
        if (bytes_read == 0) {
            break;
        }

        bytes_written = fwrite(buffer, 1, bytes_read, out);
        if (bytes_written != bytes_read) {
            return WOWS_ERROR_UNKNOWN;
        }

        total_bytes_read += bytes_read;
    }

    if (total_bytes_read != size) {
        return WOWS_ERROR_UNKNOWN;
    }
    return 0;
}

int extract_file_inode(WOWS_CONTEXT *context, WOWS_FILE_INODE *file_inode, FILE *out_file) {
    if (file_inode->type != WOWS_INODE_TYPE_FILE) {
        return WOWS_ERROR_NOT_A_FILE;
    }
    uint32_t idx_index = file_inode->index_file_index;
    WOWS_INDEX *index = context->indexes[idx_index];
    char *pkg_file_path;
    get_pkg_filepath(index, &pkg_file_path);

    FILE *fd_pkg = fopen(pkg_file_path, "r");
    if (!fd_pkg) {
        wows_set_error_details(context, "error with opening pkg file '%s'", pkg_file_path);
        free(pkg_file_path);
        return WOWS_ERROR_FILE_OPEN_FAILURE;
    }
    free(pkg_file_path);

    WOWS_INDEX_DATA_FILE_ENTRY *entry_search = &(WOWS_INDEX_DATA_FILE_ENTRY){.metadata_id = file_inode->id};
    void *res = hashmap_get(context->file_map, &entry_search);
    if (res == NULL) {
        return WOWS_ERROR_UNKNOWN;
    }
    WOWS_INDEX_DATA_FILE_ENTRY *entry = *(WOWS_INDEX_DATA_FILE_ENTRY **)res;

    int ret = 0;
    uint64_t offset = entry->offset_pkg_data;
    uint32_t size = entry->size_pkg_data;

    // FIXME entry->type_1 and entry->type_2 are probably more than on/off switch for compression.
    // Most likely, it's something like "id of compression algorithm" + "compression level"
    // But in the real world files, so far, we only encountered:
    // * (0x0, 0x0) for uncompressed
    // * (0x5, 0x1) for deflate/RFC 1951)
    // So it's good for now.
    if (entry->type_1 == 0 && entry->type_2 == 0) {
        ret = copy_data(fd_pkg, out_file, offset, size);
        if (ret != 0) {
            fclose(fd_pkg);
            return ret;
        }
    } else {
        // TODO could probably be moved in a dedicated function (like copy_data)
        fseek(fd_pkg, offset, SEEK_SET);
        char *compressed_data = malloc(CHUNK_SIZE);
        char *uncompressed_data = malloc(CHUNK_SIZE);
        z_stream stream = {0};
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        stream.avail_in = 0;
        stream.next_in = Z_NULL;
        inflateInit2(&stream, -15);

        size_t total_read = 0;
        do {
            // Read a chunk of compressed data from the input file
            const size_t compressed_bytes_read = fread(compressed_data, 1, CHUNK_SIZE, fd_pkg);
            if (compressed_bytes_read == 0 && feof(fd_pkg)) {
                break; // Reached end of the data chunk input file
            }
            if (ferror(fd_pkg)) {
                ret = WOWS_ERROR_CORRUPTED_FILE;
                break;
            }
            total_read += compressed_bytes_read;

            // Decompress the chunk of data
            stream.next_in = (Bytef *)compressed_data;
            stream.avail_in = (uInt)compressed_bytes_read;
            do {
                stream.next_out = (Bytef *)uncompressed_data;
                stream.avail_out = CHUNK_SIZE;
                inflate(&stream, Z_NO_FLUSH);

                // Write the decompressed data to the output file
                const size_t uncompressed_bytes_written = CHUNK_SIZE - stream.avail_out;
                if (fwrite(uncompressed_data, 1, uncompressed_bytes_written, out_file) != uncompressed_bytes_written) {
                    ret = WOWS_ERROR_FILE_WRITE;
                    break;
                }
            } while (stream.avail_out == 0);
            if (total_read > size) {
                break;
            }
        } while (ret == 0);
        free(compressed_data);
        free(uncompressed_data);
        inflateEnd(&stream);
    }
    fclose(fd_pkg);
    return 0;
}

FILE *open_file_with_parents(const char *filename) {
    char *p, *sep;
    struct stat sb;

    p = strdup(filename);
    if (!p) {
        return NULL;
    }

    /* Traverse the path, creating directories as needed. */
    for (sep = strchr(p + 1, '/'); sep; sep = strchr(sep + 1, '/')) {
        *sep = '\0';
        if (stat(p, &sb) == 0) {
            if (!S_ISDIR(sb.st_mode)) {
                return NULL;
            }
        } else {
            if (mkdir(p, 0755) == -1) {
                return NULL;
            }
        }
        *sep = '/';
    }

    /* Open the file for writing. */
    FILE *file = fopen(filename, "w+");
    if (!file) {
        return NULL;
    }

    free(p);

    return file;
}

typedef struct {
    char *base_path;
    WOWS_CONTEXT *context;
    FILE *magic_fp;
} extract_context;

bool extract_recursive(const void *item, void *udata) {
    WOWS_BASE_INODE *inode = *(WOWS_BASE_INODE **)item;
    extract_context *ctx = (extract_context *)udata;

    char **parent_entries = calloc(WOWS_DIR_MAX_LEVEL, sizeof(char *));
    int depth;
    get_path_inode(inode, &depth, parent_entries);

    char *path = join_path(parent_entries, depth, inode->name);

    if (inode->type == WOWS_INODE_TYPE_FILE) {
        char *out_path = calloc(strlen(path) + strlen(ctx->base_path) + 2, sizeof(char));
        sprintf(out_path, "%s/%s", ctx->base_path, path);
        WOWS_FILE_INODE *file_inode = (WOWS_FILE_INODE *)inode;
        FILE *fd;
        if (ctx->magic_fp == NULL) {
            fd = open_file_with_parents(out_path);
        } else {
            fd = ctx->magic_fp;
        }
        free(out_path);
        if (fd > 0) {
            extract_file_inode(ctx->context, file_inode, fd);
            if (ctx->magic_fp == NULL) {
                fclose(fd);
            }
        }
    } else if (inode->type == WOWS_INODE_TYPE_DIR) {
        WOWS_DIR_INODE *dir_inode = (WOWS_DIR_INODE *)inode;

        // recursively extract the directory
        struct hashmap *map = dir_inode->children_inodes;
        hashmap_scan(map, extract_recursive, udata);
    }
    free(parent_entries);
    free(path);
    return true;
}

int wows_extract_dir(WOWS_CONTEXT *context, char *dir_path, char *out_dir_path) {
    return internal_wows_extract_dir(context, dir_path, out_dir_path, NULL);
}

int internal_wows_extract_dir(WOWS_CONTEXT *context, char *dir_path, char *out_dir_path, FILE *magic_fp) {
    WOWS_BASE_INODE *inode;
    int ret = get_inode(context, dir_path, &inode);
    if (ret != 0) {
        return ret;
    }
    extract_context ctx;
    ctx.base_path = out_dir_path;
    ctx.context = context;
    ctx.magic_fp = magic_fp;
    if (inode->type != WOWS_INODE_TYPE_DIR) {
        return WOWS_ERROR_NOT_A_DIR;
    }
    WOWS_DIR_INODE *dir_inode = (WOWS_DIR_INODE *)inode;
    struct hashmap *map = dir_inode->children_inodes;
    hashmap_scan(map, extract_recursive, &ctx);

    return 0;
}

int wows_extract_file_fp(WOWS_CONTEXT *context, char *file_path, FILE *dest) {
    WOWS_BASE_INODE *inode;
    int ret = get_inode(context, file_path, &inode);
    if (ret != 0) {
        return ret;
    }
    return extract_file_inode(context, (WOWS_FILE_INODE *)inode, dest);
}

int wows_extract_file(WOWS_CONTEXT *context, char *file_path, char *out_path) {
    FILE *fd = fopen(out_path, "w+");
    if (fd <= 0) {
        wows_set_error_details(context, "error with output file '%s', %s", out_path, strerror(errno));
        return WOWS_ERROR_FILE_OPEN_FAILURE;
    }
    int ret = wows_extract_file_fp(context, file_path, fd);
    fclose(fd);
    return ret;
}
