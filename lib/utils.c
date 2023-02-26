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

#define CHUNK 16384

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int wows_inflate(FILE *source, FILE *dest, long *read) {
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    // ret = inflateInit(&strm);
    ret = inflateInit2(&strm, -15);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            // ret = inflateSync(&strm);
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR; /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    if (ret == Z_STREAM_END) {
        *read = strm.total_in;
    }

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void wows_zerr(int ret) {
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

int wows_is_dir(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

int temp_file(char *dir, char **out) {
    FILE *stream;
    size_t len;
    stream = open_memstream(out, &len);
    fprintf(stream, "%s/bf-XXXXXXX", dir);
    fflush(stream);
    fclose(stream);
    return mkstemp(*out);
}

int filename(char *dir, long start, long end, char **out) {
    FILE *stream;
    size_t len;
    stream = open_memstream(out, &len);
    fprintf(stream, "%s/%010lX-%010lX", dir, start, end);
    fflush(stream);
    fclose(stream);
    return 0;
}

int wows_inflate_all(FILE *in, char *outdir) {
    // number of in bytes read in the last
    // successful inflate call
    long read = 0;
    // return code for inf() (inflate)
    int ret_inf = 99;
    // return code for seek()
    int ret_seek = 0;
    // offset in the input file
    long offset = 0;
    // previous offset
    long old_offset = 0;

    // Recover the size of the input
    int fd = fileno(in);
    struct stat buf;
    fstat(fd, &buf);
    off_t size = buf.st_size;

    char *tmp_file_name;
    char *file_name;
    int tmp_file_fd;
    FILE *out = NULL;
    tmp_file_fd = temp_file(outdir, &tmp_file_name);
    out = fdopen(tmp_file_fd, "w+");

    while (ret_inf != Z_OK || ret_seek == 0) {
        ret_seek = fseek(in, offset, SEEK_SET);
        offset += 1;
        ret_inf = wows_inflate(in, out, &read);
        // if (ret != Z_OK)
        //    zerr(ret);
        if (ret_inf == Z_OK) {
            fclose(out);
            fprintf(stderr, "return seek  %d\n", ret_seek);
            fprintf(stderr, "cur position %ld\n", ftell(in));
            fprintf(stderr, "read  %ld\n", read);
            old_offset = offset;
            offset += read;
            fprintf(stderr, "offset  %lX -> %lX\n", old_offset, offset);
            filename(outdir, old_offset, offset, &file_name);
            rename(tmp_file_name, file_name);
            free(file_name);
            free(tmp_file_name);
            if (offset < size) {
                tmp_file_fd = temp_file(outdir, &tmp_file_name);
                out = fdopen(tmp_file_fd, "w+");
            } else {
                break;
            }
        }
        // If we are beyond the file
        // stop
        if (offset > size) {
            break;
        }
    }
    return ret_inf;
}

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
    for (int i = 0; i < depth; i++) {
        combined_len += strlen(parent_entries[i]) + 1; // +1 for the slash
    }

    // Allocate memory for the combined path string
    char *combined_path = malloc(combined_len + strlen(name) + 1); // +1 for the null terminator
    if (!combined_path) {
        return NULL;
    }

    // Copy each path component into the combined path string with a slash in between
    size_t offset = 0;
    for (int i = 0; i < depth; i++) {
        size_t len = strlen(parent_entries[i]);
        memcpy(combined_path + offset, parent_entries[i], len);
        offset += len;
        combined_path[offset++] = '/';
    }
    memcpy(combined_path + offset, name, strlen(name));
    combined_path[combined_len + strlen(name)] = '\0';

    return combined_path;
}
