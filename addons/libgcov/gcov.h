/* KallistiOS ##version##

    gcov.h
    Copyright (C) 2025 Andy Barajas

*/

#ifndef __GCOV_H
#define __GCOV_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define GCOV_FILEPATH_MAX  1024
#define TOPN_MAX_SLOT_SIZE 8

/* 
    GCOV counter types used for profiling.

    These represent various runtime statistics collected during execution.
    Only a subset may be used for any given function.
*/
enum {
    GCOV_COUNTER_ARCS,        /* Edge (branch) execution counts */
    GCOV_COUNTER_V_INTERVAL,  /* Value profiling: interval distribution */
    GCOV_COUNTER_V_POW2,      /* Value profiling: power-of-2 histogram */
    GCOV_COUNTER_V_TOPN,      /* Value profiling: top-N most frequent values */
    GCOV_COUNTER_V_INDIR,     /* Indirect call target profiling */
    GCOV_COUNTER_AVERAGE,     /* Average value tracking (e.g., loop iterations) */
    GCOV_COUNTER_IOR,         /* Bitwise OR of all observed values */
    GCOV_TIME_PROFILER,       /* Time-based profiling (first-time execution) */
    GCOV_COUNTER_CONDS,       /* Conditional branch tracking */
    GCOV_COUNTER_PATHS,       /* Execution path profiling */
    GCOV_COUNTERS             /* Total number of counter types */
};

typedef int64_t gcov_type;

/* A key-value pair used for TopN profiling. */
typedef struct topn_ctr {
    gcov_type value; /* Observed value */
    gcov_type count; /* Number of times observed */
} topn_ctr_t;

/* A TopN counter used per function site. */
typedef struct topn_counter {
    gcov_type total;     /* Total observations */
    gcov_type list_size; /* Current number of tracked values */
    gcov_type list_idx;  /* Index into the topn_table */
} topn_counter_t;

/* Represents an indirect call record. */
typedef struct indirect_call {
    void *callee;              /* Called function address */
    topn_counter_t *ctr;       /* Associated TopN counter */
} indirect_call_t;

/* Holds all counter values for a specific counter type. */
typedef struct gcov_ctr_info {
    uint32_t num;         /* Number of values in this counter */
    gcov_type *values;    /* Pointer to the counter data */
} gcov_ctr_info_t;

/* Contains metadata and counters for one instrumented function. */
typedef struct gcov_fn_info {
    const struct gcov_info *key;  /* COMDAT deduplication key */
    uint32_t ident;               /* Function ID */
    uint32_t lineno_checksum;     /* Checksum of source line numbers */
    uint32_t cfg_checksum;        /* Checksum of control flow graph */
    gcov_ctr_info_t ctrs[1];      /* One entry per used counter type */
} gcov_fn_info_t;

typedef void (*gcov_merge_fn_t)(gcov_type *counters, uint32_t num_counters);

/* 
    Holds profiling data for an entire object file.

    One of these is emitted per object file when built with --coverage,
    and registered at runtime with __gcov_init().

    This structure links together all functions and merge routines.
*/
typedef struct gcov_info {
    uint32_t version;                     /* GCOV format version */
    struct gcov_info *next;               /* Next entry in global list */
    uint32_t stamp;                       /* Timestamp of object file */
    uint32_t checksum;                    /* CRC of associated source file */
    const char *filepath;                 /* Path to the source file */
    gcov_merge_fn_t merge[GCOV_COUNTERS]; /* Merge functions per counter type */
    uint32_t num_functions;               /* Number of instrumented functions */
    const gcov_fn_info_t *const *functions; /* Pointer to array of function info structs */
} gcov_info_t;

/* 
    Builds a .gcda output path from the source file path.

    This uses the GCOV_PREFIX and GCOV_PREFIX_STRIP environment variables,
    removes the source extension, and appends ".gcda".

    Example:
      Input:  "/src/game.c"
      Output: "/pc/src/game.gcda"
*/
void gcov_build_filepath(const char *src_path, char *out_path);

/* 
    Writes profiling data for a single gcov_info object to its .gcda file.

    This function serializes all counter data (e.g., arcs, value profiles, TopN)
    for a given object file and writes it to the appropriate .gcda file on disk.

    It handles:
      - Opening the file (using filepath, GCOV_PREFIX, etc.)
      - Writing GCDA file headers and tags
      - Calling the appropriate write routines for each counter type
*/
void dump_info(const gcov_info_t *info);

/* 
    Resets all counters in a gcov_info object to zero.

    It is used after flushing data, or if you want to restart profiling
    mid-run (e.g., between levels or frames).
*/
void reset_info(gcov_info_t *info);

/* 
    Allocates and initializes the TopN profiling pool for all counters.

    This function traverses all functions in the gcov_info list and:
      - Detects TopN and indirect call counters
      - Initializes each TopN counter block (total, list_size, list_idx)
      - Calculates the number of total slots needed
      - Allocates a single aligned memory pool (topn_table)

    This must be called before any TopN counters are used or merged.
*/
void topn_pool_init(const gcov_info_t *head);

/* Frees the global memory pool allocated for TopN profiling. */
void topn_pool_shutdown(void);

/* Adds a new value observation to a TopN counter.

    If the value already exists, its count is incremented.
    If not:
      - The value is inserted if there is room
      - If full, the lowest-scoring value may be evicted based on
        count.
*/
void topn_add_value(topn_counter_t *ctr, gcov_type value, gcov_type count);

/*
    Writes a gcov_ctr_info_t containing TopN counters to a .gcda file.

    This function serializes:
      - The total observation count
      - The number of entries in the TopN list
      - Each value/count pair currently tracked
*/
void topn_write(FILE *f, const gcov_ctr_info_t *ci_ptr);

#endif /* __GCOV_H */
