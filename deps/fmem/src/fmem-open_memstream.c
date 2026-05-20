#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmem.h"

/* Store mem, size, and the FILE* so fmem_mem can fflush before reading back */
struct fmem_open_impl {
    char   *mem;
    size_t  size;
    FILE   *fp;
};

union fmem_conv {
    fmem                 *fm;
    struct fmem_open_impl *impl;
};

void fmem_init(fmem *file)
{
    union fmem_conv cv = { .fm = file };
    memset(cv.impl, 0, sizeof(*cv.impl));
}

void fmem_term(fmem *file)
{
    union fmem_conv cv = { .fm = file };
    free(cv.impl->mem);
}

FILE *fmem_open(fmem *file, const char *mode)
{
    (void) mode;

    union fmem_conv cv = { .fm = file };
    free(cv.impl->mem);
    cv.impl->mem  = NULL;
    cv.impl->size = 0;
    cv.impl->fp   = open_memstream(&cv.impl->mem, &cv.impl->size);
    return cv.impl->fp;
}

void fmem_mem(fmem *file, void **mem, size_t *size)
{
    union fmem_conv cv = { .fm = file };
    /* open_memstream only updates mem/size on fflush or fclose */
    if (cv.impl->fp)
        fflush(cv.impl->fp);
    *mem  = cv.impl->mem;
    *size = cv.impl->size;
}
