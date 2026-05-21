#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmem.h"

/* Store mem, size, FILE*, and a stable copy returned by fmem_mem.
 * open_memstream may realloc (and move) mem on fclose, so callers must not
 * use the pointer returned by fmem_mem after fclose.  We return a malloc'd
 * copy instead so it survives fclose unchanged. */
struct fmem_open_impl {
    char *mem; /* managed by open_memstream — may move on fclose */
    size_t size;
    FILE *fp;
    char *copy; /* stable malloc copy returned by fmem_mem */
};

union fmem_conv {
    fmem *fm;
    struct fmem_open_impl *impl;
};

void fmem_init(fmem *file) {
    union fmem_conv cv = {.fm = file};
    memset(cv.impl, 0, sizeof(*cv.impl));
}

void fmem_term(fmem *file) {
    union fmem_conv cv = {.fm = file};
    free(cv.impl->mem);
    free(cv.impl->copy);
}

FILE *fmem_open(fmem *file, const char *mode) {
    (void)mode;

    union fmem_conv cv = {.fm = file};
    free(cv.impl->mem);
    free(cv.impl->copy);
    cv.impl->mem = NULL;
    cv.impl->size = 0;
    cv.impl->copy = NULL;
    cv.impl->fp = open_memstream(&cv.impl->mem, &cv.impl->size);
    return cv.impl->fp;
}

void fmem_mem(fmem *file, void **mem, size_t *size) {
    union fmem_conv cv = {.fm = file};
    /* open_memstream only updates mem/size on fflush or fclose */
    if (cv.impl->fp)
        fflush(cv.impl->fp);
    /* open_memstream may realloc (and move) mem on the subsequent fclose, so
     * return a malloc'd copy that is stable across fclose.  Always allocate at
     * least 1 byte so callers see a non-NULL pointer even for an empty stream,
     * matching open_memstream's own behaviour. */
    free(cv.impl->copy);
    cv.impl->copy = malloc(cv.impl->size + 1);
    if (cv.impl->copy) {
        if (cv.impl->mem && cv.impl->size > 0)
            memcpy(cv.impl->copy, cv.impl->mem, cv.impl->size);
        ((char *)cv.impl->copy)[cv.impl->size] = '\0';
    }
    *mem = cv.impl->copy;
    *size = cv.impl->size;
}
