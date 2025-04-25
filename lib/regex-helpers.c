#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include "wows-depack.h"
#include "wows-depack-private.h"

pcre2_code *compile_regex(const char *pattern) {
    int errorcode;
    PCRE2_SIZE erroroffset;
    pcre2_code *regex;

    regex = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 0, &errorcode, &erroroffset, NULL);

    if (regex == NULL) {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", erroroffset, buffer);
        return NULL;
    }

    return regex;
}

bool match_regex(pcre2_code *regex, const char *subject) {
    pcre2_match_data *match_data;
    int rc;

    match_data = pcre2_match_data_create_from_pattern(regex, NULL);
    if (match_data == NULL) {
        fprintf(stderr, "Failed to create PCRE2 match data\n");
        return -1;
    }

    rc = pcre2_match(regex, (PCRE2_SPTR)subject, PCRE2_ZERO_TERMINATED, 0, 0, match_data, NULL);

    pcre2_match_data_free(match_data);

    if (rc < 0) {
        switch (rc) {
        case PCRE2_ERROR_NOMATCH:
            return 0;
        default:
            fprintf(stderr, "Matching error %d\n", rc);
            return -1;
        }
    }

    return 1;
}

int free_regex(pcre2_code *regex) {
    if (regex != NULL) {
        pcre2_code_free(regex);
    }
    return 0;
}
