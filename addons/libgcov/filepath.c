/* KallistiOS ##version##

    filepath.c
    Copyright (C) 2025 Andy Barajas

*/

#include <string.h>

#include <gcov/gcov.h>
#include "gcov.h"

/* 
    Strips the first `strip_count` components from a given file path,
    normalizes redundant slashes, and writes the result into `out`.
*/
void strip_leading_dirs(const char *path, int strip_count, char *out, size_t out_size) {
    const char *p = path;
    size_t i;
    bool last_was_slash = false;     // Flag to track consecutive slashes

    /* Skip any leading slashes */
    while(*p == '/') p++;

     /* Strip the specified number of path components */
    while(strip_count-- > 0) {
        /* Skip over one component (up to the next '/') */
        while(*p && *p != '/') p++;

        /* Skip the slash separator(s) */
        while(*p == '/') p++;
    }

    /* Copy the remaining path to `out`, collapsing multiple slashes into one */
    for(i = 0; *p && i < out_size - 1; ++p) {
        if(*p == '/') {
            if(last_was_slash) continue; /* Skip redundant slashes */
            last_was_slash = true;
        } 
        else
            last_was_slash = false;

        out[i++] = *p;
    }

    /* Null terminate */
    out[i] = '\0';
}

/* 
    Builds a final output path by combining a prefix (from GCOV_PREFIX) and
    a stripped version of the source path (based on GCOV_PREFIX_STRIP).
*/
void gcov_build_filepath(const char *src_path, char *out_path) {
    char *stripped = (char *)alloca(GCOV_FILEPATH_MAX);
    char *prefix_clean = (char *)alloca(GCOV_FILEPATH_MAX);

    const char *strip_str = getenv(GCOV_PREFIX_STRIP);
    int prefix_strip = strip_str ? atoi(strip_str) : 0;

    /* Strip path components from the source file path */
    strip_leading_dirs(src_path, prefix_strip, stripped, GCOV_FILEPATH_MAX);

    const char *prefix = getenv(GCOV_PREFIX);
    if(!prefix)
        prefix = "";

    /* Remove trailing slash from the prefix if it exists */
    size_t prefix_len = strlen(prefix);
    if(prefix_len > 0 && prefix[prefix_len - 1] == '/') {
        /* Copy all but the trailing slash into prefix_clean */
        snprintf(prefix_clean, GCOV_FILEPATH_MAX, "%.*s", (int)(prefix_len - 1), prefix);
        prefix = prefix_clean;
    }

    /* Final output path: prefix + '/' + stripped, or just stripped */
    if(*prefix)
        snprintf(out_path, GCOV_FILEPATH_MAX, "%s/%s", prefix, stripped);
    else
        snprintf(out_path, GCOV_FILEPATH_MAX, "%s", stripped);
}
