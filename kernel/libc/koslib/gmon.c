/* KallistiOS ##version##

    gmon.c
    Copyright (C) 2025 Andress Barajas

    The methods provided in this file are based on the principles outlined in
    the gprof profiling article. However, instead of utilizing a linked list
    for storing profiling data, a binary search tree (BST) is used instead.

    Article:
    (https://mcuoneclipse.com/2015/08/23/tutorial-using-gnu-profiling-gprof-with-arm-cortex-m/)

*/

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <sys/gmon.h>
#include <kos/thread.h>
#include <kos/dbglog.h>

#define ROUNDDOWN(x,y)	(((x)/(y))*(y))
#define ROUNDUP(x,y)	((((x)+(y)-1)/(y))*(y))

#define HISTOGRAM_INTERVAL_MS  10

/* The type that holds the count for the histogram every 10 ms */
#define	HIST_COUNTER_TYPE      uint16_t

/* Number of histogram buckets = text_size / HISTFRACTION */
#define HISTFRACTION    8

/* Hash-table reduction factor:
   froms[] slots = text_size / HASHFRACTION */
#define	HASHFRACTION   16

/* Call-graph arc density as a percentage:
   number of arcs ≈ (text_size * ARCDENSITY) / 100 */
#define ARCDENSITY	    2

#define MAX_NODES   65535

#define	GMON_MAGIC	   "gmon"
#define GMON_VERSION	    1
#define GMON_TAG_TIME_HIST  0
#define GMON_TAG_CG_ARC     1

#define GMON_OUT_PATH   "/pc/gmon.out"

/* GMON file header  */
typedef struct gmon_hdr {
    char cookie[4];
    int32_t version;
    char spare[3 * 4];
} gmon_hdr_t;

typedef struct gmon_hist_hdr {
    uintptr_t lowpc;
    uintptr_t highpc;
    uint32_t num_bins;
    uint32_t profrate;
    char units[15];
    char units_abbrev;
} gmon_hist_hdr_t;

/* GMON arc */
typedef struct gmon_arc {
    uintptr_t frompc;
    uintptr_t selfpc;
    size_t count;
} gmon_arc_t;

/* BST node */
typedef struct gmon_node {
    uintptr_t selfpc;
    uint32_t count;
    uint16_t left;
    uint16_t right;
} gmon_node_t;

/* GMON profiling states */
typedef enum gmon_state {
    GMON_PROF_ON,
    GMON_PROF_BUSY,
    GMON_PROF_ERROR,
    GMON_PROF_OFF
} gmon_state_t;

/* GMON context */
typedef struct gmon_context {
    gmon_state_t state;
    uintptr_t lowpc;
    uintptr_t highpc;
    size_t textsize;

    /* Array of indices to heads of BST */
    size_t nfroms;
    uint16_t *froms;

    /* BST */
    size_t nnodes;
    gmon_node_t *nodes;
    size_t node_count;

    size_t allocate_size;

    /* Histogram */
    size_t ncounters;
    HIST_COUNTER_TYPE *histogram;

    kthread_t *main_thread;
    kthread_t *histogram_thread;

    volatile bool running_thread;
    volatile bool initialized;
} gmon_context_t;

static gmon_context_t g_context = {
    .state = GMON_PROF_OFF,
    .lowpc = 0,
    .highpc = 0,
    .textsize = 0,
    .nfroms = 0,
    .froms = NULL,
    .nnodes = 0,
    .nodes = NULL,
    .node_count = 0,
    .ncounters = 0,
    .allocate_size = 0,
    .histogram = NULL,
    .main_thread = NULL,
    .histogram_thread = NULL,
    .running_thread = false,
    .initialized = false
};

/* Function to write the arcs to gmon.out */
static void traverse_and_write(FILE *out, gmon_context_t *cxt, uint32_t index, uintptr_t from_addr) {
    gmon_arc_t arc;
    gmon_node_t *node;
    uint8_t tag = GMON_TAG_CG_ARC;

    if(index == 0)
        return;

    /* Grab a node */
    node = &cxt->nodes[index];

    /* Traverse the left subtree */
    traverse_and_write(out, cxt, node->left, from_addr);

    /* Write the arc */
    arc.frompc = from_addr;
    arc.selfpc = node->selfpc;
    arc.count = node->count;
    if(fwrite(&tag, sizeof(tag), 1, out) != 1 ||
       fwrite(&arc, sizeof(gmon_arc_t), 1, out) != 1) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write arc for %p → %p\n", (void *)from_addr, (void *)arc.selfpc);
        return;
    }

    /* Traverse the right subtree */
    traverse_and_write(out, cxt, node->right, from_addr);
}

static bool write_histogram(void) {
    gmon_context_t *cxt = &g_context;
    FILE *out = fopen(GMON_OUT_PATH, "a");

    if(!out) {
        dbglog(DBG_ERROR, "[GPROF] Failed to open /pc/gmon.out for histogram.\n");
        return false;
    }

    /* Write Histogram Tag */
    uint8_t tag = GMON_TAG_TIME_HIST;
    if(fwrite(&tag, sizeof(tag), 1, out) != 1) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write histogram tag.\n");
        fclose(out);
        return false;
    }

    /* Write Histogram Header */
    gmon_hist_hdr_t hist_hdr = {
        .lowpc = cxt->lowpc,
        .highpc = cxt->highpc,
        .num_bins = cxt->ncounters,
        .profrate = HISTOGRAM_INTERVAL_MS,
        .units = "seconds",
        .units_abbrev = 's'
    };
    if(fwrite(&hist_hdr, sizeof(hist_hdr), 1, out) != 1) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write histogram header.\n");
        fclose(out);
        return false;
    }

    /* Write Histogram Data */
    size_t written = fwrite(cxt->histogram, sizeof(HIST_COUNTER_TYPE), cxt->ncounters, out);
    if(written != cxt->ncounters) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write complete histogram data (%u/%u bins).\n",
               (unsigned)written, (unsigned)cxt->ncounters);
        fclose(out);
        return false;
    }

    fclose(out);

    return true;
}

static bool write_arcs(void) {
    uintptr_t from_addr;
    uint32_t from_index;
    gmon_context_t *cxt = &g_context;

    FILE *out = fopen(GMON_OUT_PATH, "a");
    if(!out) {
        dbglog(DBG_ERROR, "[GPROF] Failed to open /pc/gmon.out for arcs.\n");
        return false;
    }

    /* Write Arcs */
    for(from_index = 0; from_index < cxt->nfroms; from_index++) {
        /* Skip if no BST built for this index */
        if(cxt->froms[from_index] == 0)
            continue;

        /* Construct 'from' address by performing reciprical of hash to insert */
        from_addr = (HASHFRACTION * from_index) + cxt->lowpc;

        /* Write out the arcs in the BST */
        traverse_and_write(out, cxt, cxt->froms[from_index], from_addr);
    }

    fclose(out);

    return true;
}

/* Function that gets executed every 10ms */
static void *histogram_thread(void *arg) {
    (void)arg;
    uintptr_t pc;
    uint32_t index;
    gmon_context_t *cxt = &g_context;

    while(cxt->running_thread) {
        if(cxt->state == GMON_PROF_ON) {
            /* Grab the PC of the kernel thread */
            pc = cxt->main_thread->context.pc;

            /* If function is within the .text section */
            if(pc >= cxt->lowpc && pc <= cxt->highpc) {
                /* Compute the index in the histogram */
                index = (pc - cxt->lowpc) / HISTFRACTION;

                /* Increment the histogram counter */
                cxt->histogram[index]++;
            }
        }

        /* Always sleep for 10ms before the next sample */
        thd_sleep(HISTOGRAM_INTERVAL_MS);
    }

    return NULL;
}

/* Function to enable/disable gprofiling */
void moncontrol(bool enable) {
    gmon_context_t *cxt = &g_context;

    /* Don't change the state if we ran into an error */
    if(cxt->state == GMON_PROF_ERROR)
        return;

    /* Start only if enabled and initialized */
    if(enable && cxt->initialized)
        /* Start */
        cxt->state = GMON_PROF_ON;
    else
        /* Stop */
        cxt->state = GMON_PROF_OFF;
}

/* Called to cleanup and generate gmon.out file */
static void _mcleanup(void) {
    gmon_context_t *cxt = &g_context;

    /* Stop profiling, stop running histogram thread and join */
    moncontrol(false);
    cxt->running_thread = false;
    if(cxt->histogram_thread != NULL)
        thd_join(cxt->histogram_thread, NULL);

    /* Dont output if uninitialized or we encountered an error */
    if(!cxt->initialized || cxt->state == GMON_PROF_ERROR) {
        dbglog(DBG_ERROR, "_mcleanup: uninitialized or we encountered an error. \n");
        goto cleanup;
    }

    if(!write_histogram())
        goto cleanup;

    if(cxt->nfroms > 0 && !write_arcs())
        goto cleanup;

cleanup:
    if(cxt->histogram) {
        free(cxt->histogram);
        cxt->histogram = NULL;
    }

    /* Reset buffer to initial state for safety */
    memset(cxt, 0, sizeof(g_context));

    /* Somewhat confusingly, ON=0, OFF=3 */
    cxt->state = GMON_PROF_OFF;
}

/* Called each time we enter a function */
static inline void _mcount(uintptr_t frompc, uintptr_t selfpc) {
    uint32_t index;
    uint16_t node_index;
    uint16_t *index_ptr;
    gmon_node_t *node;
    size_t arc_buf_size;
    gmon_context_t *cxt = &g_context;

    /* Exit early if profiling isnt turned on */
    if(cxt->state != GMON_PROF_ON)
        return;

    /* Exit early if function is outside the .text section */
    if(frompc < cxt->lowpc || frompc > cxt->highpc)
        return;

    /* Calculate index into 'froms' array */
    index = (frompc - cxt->lowpc) / HASHFRACTION;
    index_ptr = &cxt->froms[index];

    /* Grab index into node array */
    node_index = *index_ptr;

    /* Node doesnt exist? */
    if(node_index == 0)
        goto create_node;

    /* Try and find the node */
    node = &cxt->nodes[node_index];

    while(true) {
        /* Found the node */
        if(node->selfpc == selfpc) {
            node->count++;
            return;
        }
        /* Check if selfpc is less */
        else if(selfpc < node->selfpc) {
            if(node->left == 0) {
                index_ptr = &node->left;
                goto create_node;
            }
            /* Visit left node */
            node = &cxt->nodes[node->left];
        }
        else {
            if(node->right == 0) {
                index_ptr = &node->right;
                goto create_node;
            }
            /* Visit right node */
            node = &cxt->nodes[node->right];
        }
    }

create_node:
    /* 'Allocate' a node */
    node_index = ++cxt->node_count;
    if(node_index >= cxt->nnodes)
        goto overflow;

    *index_ptr = node_index;
    node = &cxt->nodes[node_index];
    node->selfpc = selfpc;
    node->count = 1;
    return;

overflow:
    /* Pause profiling to write Arcs */
    cxt->state = GMON_PROF_OFF;

    if(!write_arcs()) {
        cxt->state = GMON_PROF_ERROR;
        _mcleanup();
        return;
    }

    /* Reset Arcs only */
    arc_buf_size = cxt->allocate_size - (cxt->ncounters * sizeof(HIST_COUNTER_TYPE));
    memset(cxt->froms, 0, arc_buf_size);
    cxt->node_count = 0;

    /* Restart profiling */
    cxt->state = GMON_PROF_ON;
}

/* Called to setup gprof profiling */
static void _monstartupbase(uintptr_t lowpc, uintptr_t highpc, bool generate_callgraph) {
    size_t counter_size;
    size_t froms_size;
    size_t nodes_size;
    gmon_context_t *cxt = &g_context;

    /* Exit early if we already initialized */
    if(cxt->initialized) {
        dbglog(DBG_NOTICE, "_monstartup: Already initialized.\n");
        return;
    }

    cxt->lowpc = ROUNDDOWN(lowpc, HISTFRACTION * sizeof(HIST_COUNTER_TYPE));
    cxt->highpc = ROUNDUP(highpc, HISTFRACTION * sizeof(HIST_COUNTER_TYPE));
    cxt->textsize = cxt->highpc - cxt->lowpc;

    /* Calculate the number of counters, rounding up to ensure no remainder,
       ensuring that all of textsize is covered without any part being left out */
    cxt->ncounters = (cxt->textsize + HISTFRACTION - 1) / HISTFRACTION;
    counter_size = cxt->ncounters * sizeof(HIST_COUNTER_TYPE);

    if(generate_callgraph) {
        cxt->nfroms = (cxt->textsize + HASHFRACTION - 1) / HASHFRACTION;
        froms_size = cxt->nfroms * sizeof(uint16_t);

        cxt->nnodes = (cxt->textsize * ARCDENSITY) / 100;
        if(cxt->nnodes > MAX_NODES) cxt->nnodes = MAX_NODES;
        nodes_size = cxt->nnodes * sizeof(gmon_node_t);

        /* Allocate enough so all buffers can be 32-byte aligned */
        cxt->allocate_size = ROUNDUP(counter_size, 32) +
                             ROUNDUP(froms_size, 32) +
                             ROUNDUP(nodes_size, 32) +
                             32; /* Extra space for alignment adjustments */
    }
    else {
        /* Just allocate enough for the histogram */
        cxt->allocate_size = ROUNDUP(counter_size, 32);
    }

    dbglog(DBG_NOTICE, "[GPROF] Profiling from <%p to %p>\n"
        "[GPROF] Range size: %d bytes\n",
        (void *)cxt->lowpc, (void *)cxt->highpc, cxt->textsize);

    if(posix_memalign((void**)&cxt->histogram, 32, cxt->allocate_size)) {
        cxt->state = GMON_PROF_ERROR;
        dbglog(DBG_ERROR, "_monstartup: Unable to allocate memory.\n");
        return;
    }

    dbglog(DBG_NOTICE, "[GPROF] Total memory allocated: %d bytes\n\n", cxt->allocate_size);

    /* Clear and align the other buffers to 32 bytes */
    memset(cxt->histogram, 0, cxt->allocate_size);
    if(generate_callgraph) {
        cxt->froms = (uint16_t *)((uintptr_t)cxt->histogram + ROUNDUP(counter_size, 32));
        cxt->nodes = (gmon_node_t *)((uintptr_t)cxt->froms + ROUNDUP(froms_size, 32));
    }

    /* Create gmon.out */
    FILE *out = fopen(GMON_OUT_PATH, "w");
    if(!out) {
        dbglog(DBG_ERROR, "/pc/gmon.out not opened.\n");
        return;
    }

    /* Write GMON header */
    gmon_hdr_t hdr = {
        .cookie = { 'g', 'm', 'o', 'n' },
        .version = 1,
        .spare = { 0 }
    };
    if(fwrite(&hdr, sizeof(hdr), 1, out) != 1) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write GMON header.\n");
        fclose(out);
        return;
    }

    fclose(out);

    /* Initialize histogram thread related members */
    cxt->running_thread = true;
    cxt->main_thread = thd_by_tid(KOS_PID);
    cxt->histogram_thread = thd_create(false, histogram_thread, NULL);
    thd_set_prio(cxt->histogram_thread, PRIO_DEFAULT / 2);
    thd_set_label(cxt->histogram_thread, "histogram_thread");

    cxt->initialized = true;

    /* Cleanup at exit */
    atexit(_mcleanup);

    /* Turn profiling on */
    moncontrol(true);
}

/* Calling this function will not generate a callgraph */
void monstartup(uintptr_t lowpc, uintptr_t highpc) {
    _monstartupbase(lowpc, highpc, false);
}

/*
 * Profiling trap-#33 handler.
 *
 * GCC emits the instrumentation sequence at each function entry as:
 *
 *   Offset    Bytes     Meaning
 *   ----------------------------------------
 *     +0      2 bytes   trapa #33
 *     +2      2 bytes   nop              <- context->pc points here
 *     +4      4 bytes   .long LABELNO
 *   ----------------------------------------
 *
 * On entry, context->pc == original_trap_addr + 2 (the NOP).
 * To resume at the real code (+8), we need to skip:
 *   2 bytes of NOP  +  4 bytes of the LABELNO constant  =  6 bytes
 *
 * Hence in our handler we do:
 *     context->pc + 6;   // moves PC from +2 → +8
 */
static void handle_gprof_trapa(trapa_t code, irq_context_t *context, void *data) {
    (void)code;
    (void)data;

    uint32_t resume_pc = context->pc + 6;

    /* Patch the saved PC so RTE returns to real code */
    context->pc = resume_pc;

    _mcount(context->pr, resume_pc);
}

/* Calling this function will generate a callgraph. Called in gcrt1.S */
void _monstartup(uintptr_t lowpc, uintptr_t highpc) {
    _monstartupbase(lowpc, highpc, true);

    /* Setup handler to catch trapa #33 */
    trapa_set_handler(GPROF_TRAPA_CODE, handle_gprof_trapa, NULL);
}
