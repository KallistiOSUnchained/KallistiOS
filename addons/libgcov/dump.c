/* KallistiOS ##version##

    dump.c
    Copyright (C) 2025 Andy Barajas

*/

#include <kos/dbglog.h>

#include "gcov.h"

#define GCOV_TAG_EOF                0

#define GCOV_MAGIC                  ((uint32_t)0x67636461) /* "gcda" */
#define GCOV_TAG_FUNCTION           ((uint32_t)0x01000000)
#define GCOV_TAG_COUNTER_BASE       0x01a10000
#define GCOV_TAG_FOR_COUNTER(CNT)   (GCOV_TAG_COUNTER_BASE + ((uint32_t)(CNT) << 17))
#define GCOV_COUNTER_FOR_TAG(TAG)   (((TAG) - GCOV_TAG_COUNTER_BASE) >> 17)

/* So merge function pointers can have access to data we want to merge */
gcov_type *merge_temp_buf = NULL;

static inline bool are_all_counters_zero(const gcov_ctr_info_t *ci_ptr) {
    for(uint32_t i = 0; i < ci_ptr->num; i++) {
        if(ci_ptr->values[i] != 0)
            return false;
    }

    return true;
}

/* Reads a .gcda file and merges its counters into the current gcov_info_t */
static void merge_existing_gcda(const gcov_info_t *info) {
    FILE *f;
    int32_t length;
    uint32_t tag;
    uint32_t fn_idx = 0;
    uint32_t ctr_idx = 0;
    char out_path[GCOV_FILEPATH_MAX];

    gcov_build_filepath(info->filepath, out_path);

    f = fopen(out_path, "rb");
    if(!f)
        return;

    /* Check MAGIC */
    if(fread(&tag, 4, 1, f) != 1 || tag != GCOV_MAGIC)
        goto done;

    /* Skip version, stamp, and checksum */
    fseek(f, 12, SEEK_CUR); 

    /* Read TAG blocks */
    while(fread(&tag, 4, 1, f) == 1) {
        if(tag == GCOV_TAG_EOF)
            break;

        /* Read TAG length (in bytes) */
        if(fread(&length, 4, 1, f) != 1)
            break;

        if(tag == GCOV_TAG_FUNCTION) {
            /* Skip ident, lineno_checksum, and cfg_checksum */
            fseek(f, length, SEEK_CUR); 
            fn_idx++;
            /* Reset counters index for new function */
            ctr_idx = 0;
        } 
        else if(tag >= GCOV_TAG_FOR_COUNTER(0) &&
                tag < GCOV_TAG_FOR_COUNTER(GCOV_COUNTERS)) {

            /* Skip if negative - Counters are all zero */
            if(length < 0) {
                ctr_idx++;
                continue;
            } 

            int mrg_idx = GCOV_COUNTER_FOR_TAG(tag);
            const gcov_fn_info_t *fn = info->functions[fn_idx - 1];

            if(fn && info->merge[mrg_idx]) {
                gcov_type *buffer = NULL;
                if(posix_memalign((void **)&buffer, 8, length))
                    break;

                if(fread(buffer, length, 1, f) != 1) {
                    free(buffer);
                    break;
                }

                uint32_t num_counters = length / sizeof(gcov_type);
                merge_temp_buf = buffer;
                info->merge[mrg_idx](fn->ctrs[ctr_idx].values, num_counters);
                merge_temp_buf = NULL;
                free(buffer);
            }
            else {
                /* This shouldnt happen */
                fseek(f, length, SEEK_CUR);
            } 

            ctr_idx++;
        } 
        else {
            /* Skip unknown tag block */
            fseek(f, length, SEEK_CUR);
        }
    }
    dbglog(DBG_ERROR, "GCOV: Done merging file%s\n", out_path);
done:
    fclose(f);
}

static void write_gdca(const gcov_info_t *info) {
    FILE *f;
    bool all_zero;
    uint32_t i, j, k, tmp, length;
    char out_path[GCOV_FILEPATH_MAX];

    gcov_build_filepath(info->filepath, out_path);

    f = fopen(out_path, "wb");
    if(!f) {
        dbglog(DBG_ERROR, "GCOV: Failed to open %s\n", out_path);
        return;
    }

    /* File header */
    tmp = GCOV_MAGIC;
    fwrite(&tmp, 1, 4, f);
    fwrite(&info->version, 1, 4, f);
    fwrite(&info->stamp, 1, 4, f);
    fwrite(&info->checksum, 1, 4, f);

    /* Each function */
    for(i = 0; i < info->num_functions; ++i) {
        const gcov_fn_info_t *fn = info->functions[i];
        length = (fn && fn->key == info) ? 12 : 0;

        /* Tag: Function header */
        tmp = GCOV_TAG_FUNCTION;
        fwrite(&tmp, 1, 4, f);
        fwrite(&length, 1, 4, f); /* # of following bytes */
        if(!length) continue;

        fwrite(&fn->ident, 1, 4, f);
        fwrite(&fn->lineno_checksum, 1, 4, f);
        fwrite(&fn->cfg_checksum, 1, 4, f);

        /* Each Counter type */
        const gcov_ctr_info_t *ci = fn->ctrs;
        for(j = 0; j < GCOV_COUNTERS; ++j) {
            if(!info->merge[j])
                continue;
            
            tmp = GCOV_TAG_FOR_COUNTER(j);
            fwrite(&tmp, 1, 4, f);

            if(j == GCOV_COUNTER_V_TOPN || j == GCOV_COUNTER_V_INDIR)
                topn_write(f, ci);
            else {
                all_zero = are_all_counters_zero(ci);

                tmp = sizeof(gcov_type) * (all_zero ? -ci->num : ci->num);
                fwrite(&tmp, 1, 4, f);

                if(!all_zero) {
                    for(k = 0; k < ci->num; ++k) {
                        fwrite(&ci->values[k], 1, sizeof(gcov_type), f);
                    }
                }
            }

            ci++;
        }
    }

    /* Terminator */
    tmp = GCOV_TAG_EOF;
    fwrite(&tmp, 1, 4, f);

    fclose(f);

    dbglog(DBG_NOTICE, "GCOV: dumped %s\n", out_path);
}

void dump_info(const gcov_info_t *info) {
    merge_existing_gcda(info);
    write_gdca(info);
}
