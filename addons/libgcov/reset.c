/* KallistiOS ##version##

    reset.c
    Copyright (C) 2025 Andy Barajas

*/

#include "gcov.h"

/*
    Zeroes out all active counters associated with each function in 
    the provided gcov_info_t structure.
 */
void reset_info(gcov_info_t *info) {
    uint32_t i, j, k;

    /* Each function */
    for(i = 0; i < info->num_functions; ++i) {
        const gcov_fn_info_t *fn = info->functions[i];
        if(!fn) continue;

        /* Each Counter type */
        const gcov_ctr_info_t *ctr = fn->ctrs;
        for(j = 0; j < GCOV_COUNTERS; ++j) {
            /* Skip counter type if not present.
              'merge[j]' is only set for active counters */
            if(!info->merge[j])
                continue;

            if(ctr->num > 0 && ctr->values) {
                for(k = 0; k < ctr->num; ++k) {
                    ctr->values[k] = 0;
                }
            }

            ctr++;
        }
    }
}
