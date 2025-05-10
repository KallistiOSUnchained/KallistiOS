/* KallistiOS ##version##

    merge.c
    Copyright (C) 2025 Andy Barajas

*/

#include <kos/dbglog.h>

#include <gcov/gcov.h>
#include "gcov.h"

extern gcov_type *merge_temp_buf;

/*
    Merges profiling data for TopN value counters. Each counter block tracks 
    the most frequently observed values (key-value pairs) with a max count of
    TOPN_MAX_SLOT_SIZE.

    Serialized memory layout in merge_temp_buf:
    - counters[0]: Total observations (optional; currently skipped)
    - counters[1]: Number of entries (N)
    - counters[2..]: N pairs of (value, count)

    Destination counters[] layout:
    - counters[0]: Total observations
    - counters[1]: Number of entries in the linked list
    - counters[2]: Index of the head entry in the topn_table

    IMPORTANT:
    - num_counters: Must be a multiple of 3 for valid TopN counter blocks.
*/
void __gcov_merge_topn(gcov_type *counters, uint32_t num_counters) {
    if(!merge_temp_buf || (num_counters % 3) != 0) {
        dbglog(DBG_ERROR, "GCOV: __gcov_merge_topn: invalid state\n");
        return;
    }

    uint32_t i, j;
    gcov_type *buf_ptr = merge_temp_buf;
    gcov_type value, count, num_entries;

    /* Interpret counters as an array of topn_counter_t structs */
    topn_counter_t *counter = (topn_counter_t *)counters;

    /* topn_counter_t structs (3 gcov_type each) */
    num_counters /= 3;

    for(i = 0; i < num_counters; i++) {
        /* Skip Total; Handled in topn_add_value */
        buf_ptr++;

        num_entries = *buf_ptr++;

        for(j = 0; j < num_entries; j++) {
            value = *buf_ptr++;
            count = *buf_ptr++;
            topn_add_value(counter, value, count);
        }

        counter++; 
    }
}

/* 
    Merges profiling counters that track the timestamp of the first time a 
    function was entered. This merge keeps the earliest time (smallest value)
    between existing and new.
*/
void __gcov_merge_time_profile(gcov_type *counters, uint32_t num_counters) {
    uint32_t i;

    if(!merge_temp_buf) return;

    for(i = 0; i < num_counters; ++i) {
        if(merge_temp_buf[i] && 
           (counters[i] == 0 || merge_temp_buf[i] < counters[i])) {
            counters[i] = merge_temp_buf[i];
        }
    }
}

/*
    Merges profiling counters that performs element-wise addition of
    counters.
*/
void __gcov_merge_add(gcov_type *counters, uint32_t num_counters) {
    uint32_t i;

    if(!merge_temp_buf) return;

    for(i = 0; i < num_counters; ++i)
        counters[i] += merge_temp_buf[i];
}

/*
    Merges profiling counters that track the bitwise OR of all values 
    observed during execution. Each counter records a single gcov_type value 
    representing the union of flags or bits.
*/
void __gcov_merge_ior(gcov_type *counters, uint32_t num_counters) {
    uint32_t i;

    if(!merge_temp_buf) return;

    for(i = 0; i < num_counters; ++i)
        counters[i] |= merge_temp_buf[i];
}
