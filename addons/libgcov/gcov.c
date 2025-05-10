/* KallistiOS ##version##

    gcov.c
    Copyright (C) 2025 Andy Barajas

*/

#include <kos/mutex.h>
#include <kos/dbglog.h>

#include <gcov/gcov.h>
#include "gcov.h"

/* Head of the linked list of registered coverage data. */
static gcov_info_t *__gcov_info_head = NULL;

/* Flag to ensure GCOV environment setup only happens once. */
static bool set_default_env = false;

static mutex_t gcov_mutex = MUTEX_INITIALIZER;

/* Init the memory pool */
void gprof_init(void) {
    topn_pool_init(__gcov_info_head);
}

/* Register a new gcov_info block with the runtime. */
void __gcov_init(gcov_info_t *info) {
    if(!info->version || !info->num_functions)
        return;

    info->next = __gcov_info_head;
    __gcov_info_head = info;

    /* Set a default output prefix if none is already set. */
    if(!set_default_env) {
        set_default_env = true;
        setenv(GCOV_PREFIX, "/pc", true);
    }

    // gcov_fn_info_t *fn = NULL;
    // gcov_ctr_info_t *ci = NULL;

    // for(uint32_t f_ix = 0; f_ix < info->num_functions; f_ix++) {
    //     fn = info->functions[f_ix];
    //     dbglog(DBG_NOTICE, "   functions[%lu] = %p\n", f_ix, (void *)fn);
    //     dbglog(DBG_NOTICE, "     fn[%lu]: ident=0x%08lx, checksum=0x%08lx\n",
    //            f_ix, fn->ident, fn->cfg_checksum);

    //     ci = fn->ctrs;
    //     for(uint32_t t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++) {
    //         if(!info->merge[t_ix])
    //             continue;

    //         dbglog(DBG_NOTICE,
    //                "       CTR[%lu]: num=%lu, values=%p\n",
    //                t_ix, ci->num, (void *)ci->values);

    //         ci++; // move to next active counter
    //     }
    // }
}

/* Reset all coverage counters to zero. */
void __gcov_reset(void) {
    gcov_info_t *info = __gcov_info_head;

    mutex_lock_scoped(&gcov_mutex);

    while(info) {
        reset_info(info);
        info = info->next;
    }
}

/* Write out .gcda data for all registered objects. */
void __gcov_dump(void) {
    gcov_info_t *info = __gcov_info_head;

    mutex_lock_scoped(&gcov_mutex);

    while(info) {
        dump_info(info);
        info = info->next;
    }
}

/* Called on exit */
void __gcov_exit(void) {
    //topn_dump_text(__gcov_info_head);

    /* Flush all coverage data */
    __gcov_dump();

    /* Free the memory pool */
    topn_pool_shutdown();
}
