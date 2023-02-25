#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "wows-depack.h"
#include "wows-depack-private.h"

char *wows_error_string(int error_code, WOWS_CONTEXT *context) {
    char *error_string;

    switch (error_code) {
    case WOWS_ERROR_CORRUPTED_FILE:
        error_string = "The index is corrupted";
        break;
    case WOWS_ERROR_BAD_MAGIC:
        error_string = "The index has an invalid magic number";
        break;
    case WOWS_ERROR_MISSING_METADATA_ENTRY:
        error_string = "The file is missing a required metadata entry";
        break;
    case WOWS_ERROR_MAX_LEVEL_REACHED:
        error_string = "The maximum level has been reached";
        break;
    case WOWS_ERROR_NON_ZERO_TERMINATED_STRING:
        error_string = "A string in the index is not null-terminated";
        break;
    case WOWS_ERROR_PATH_TOO_LONG:
        error_string = "The file path is too long";
        break;
    case WOWS_ERROR_UNKNOWN:
        error_string = "An unknown error occurred";
        break;
    case WOWS_ERROR_ID_COLLISION_FILE_DIR:
        error_string = "There is an ID collision between a file and a directory";
        break;
    case WOWS_ERROR_FILE_OPEN_FAILURE:
        error_string = "The index could not be opened";
        break;
    case WOWS_ERROR_DECOMPOSE_PATH:
        error_string = "failed to decompose path into [dir1, dir2, etc] + file";
        break;
    case 0:
        error_string = "no recent error";
        break;
    default:
        error_string = "unrecognized error code";
        break;
    }

    // If a context argument was provided, add it to the error message.
    if (context->err_msg != NULL) {
        char *error_message = (char *)malloc(strlen(error_string) + strlen(context->err_msg) + 3);
        sprintf(error_message, "%s: %s", error_string, context->err_msg);
        return error_message;
    } else {
        char *error_message = (char *)malloc(strlen(error_string) + 1);
        sprintf(error_message, "%s", error_string);
        return error_message;
    }
}

void wows_set_error_details(WOWS_CONTEXT *context, char *fmt, ...) {
    if (context->err_msg != NULL) {
        free(context->err_msg);
    }
    FILE *stream;
    char *out;
    size_t len;
    stream = open_memstream(&out, &len);
    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);
    fflush(stream);
    fclose(stream);
    context->err_msg = out;
}
