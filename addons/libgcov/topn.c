/* KallistiOS ##version##

    topn.c
    Copyright (C) 2025 Andy Barajas

*/

#include <string.h>

#include <kos/dbglog.h>

#include <gcov/gcov.h>
#include "gcov.h"

static topn_ctr_t *topn_table = NULL;

// void topn_dump_text(const gcov_info_t *head) {
//     for (const gcov_info_t *info = head; info; info = info->next) {
//         for (uint32_t f = 0; f < info->num_functions; f++) {
//             const gcov_fn_info_t *fn = info->functions[f];
//             if (!fn) continue;

//             const gcov_ctr_info_t *ctrs = fn->ctrs;
//             uint32_t ctr_index = 0;

//             for (uint32_t c = 0; c < GCOV_COUNTERS; c++) {
//                 if (!info->merge[c]) continue;

//                 if ((c == GCOV_COUNTER_V_TOPN || c == GCOV_COUNTER_V_INDIR) &&
//                     ctrs[ctr_index].num > 0) {

//                     uint32_t num = ctrs[ctr_index].num / 3;
//                     topn_counter_t *counters = (topn_counter_t *)ctrs[ctr_index].values;

//                     for (uint32_t i = 0; i < num; i++) {
//                         dbglog(DBG_NOTICE, "\n[GCOV][TopN] Counter #%lu (slot base %llu):\n", i, counters[i].list_idx);

//                         for (uint32_t j = 0; j < counters[i].list_size; j++) {
//                             topn_ctr_t *e = &topn_table[counters[i].list_idx + j];
//                             dbglog(DBG_NOTICE, "  [%02lu] value=%llu count=%llu\n",
//                                 j, e->value, e->count);
//                         }
//                     }
//                 }

//                 ctr_index++;
//             }
//         }
//     }
// }

void topn_pool_init(const gcov_info_t *head) {
    uint32_t fn_idx, ctr_idx, ent_idx;
    uint32_t total_entries = 0;
    uint32_t active_ctr_index;
    uint32_t topn_table_size = 0;
    topn_counter_t *counter;

    for(const gcov_info_t *info = head; info; info = info->next) {
        for(fn_idx = 0; fn_idx < info->num_functions; fn_idx++) {
            const gcov_ctr_info_t *ctrs = info->functions[fn_idx]->ctrs;
            active_ctr_index = 0;

            for(ctr_idx = 0; ctr_idx < GCOV_COUNTERS; ctr_idx++) {
                if(!info->merge[ctr_idx])
                    continue;

                const gcov_ctr_info_t *ci = &ctrs[active_ctr_index];
                if((ctr_idx == GCOV_COUNTER_V_TOPN || ctr_idx == GCOV_COUNTER_V_INDIR) &&
                    ci->num > 0) {

                    uint32_t num_counters = ci->num / 3;

                    for(ent_idx = 0; ent_idx < num_counters; ent_idx++) {
                        counter = (topn_counter_t *)(&ci->values[ent_idx * 3]);
                        counter->total = 0;
                        counter->list_size = 0;
                        counter->list_idx = total_entries * TOPN_MAX_SLOT_SIZE;
                        total_entries++;
                    }
                }

                active_ctr_index++;
            }
        }
    }

    /* Allocate memory for topn_ctr_t table */
    topn_table_size = total_entries * TOPN_MAX_SLOT_SIZE * sizeof(topn_ctr_t);

    if(posix_memalign((void**)&topn_table, 32, topn_table_size))
        dbglog(DBG_ERROR, "[GCOV] Failed to allocate %lu memory for TopN table\n", topn_table_size);
    else {
        dbglog(DBG_NOTICE, "[GCOV] Allocated %lu bytes for TopN table\n", topn_table_size);
        memset(topn_table, 0, topn_table_size);
    }
}

void topn_pool_shutdown(void) {
    free(topn_table);
    topn_table = NULL;
}

static __always_inline void swap(topn_ctr_t *a, topn_ctr_t *b) {
    topn_ctr_t temp = *a;
    *a = *b;
    *b = temp;
}

void topn_add_value(topn_counter_t *ctr, gcov_type value, gcov_type count) {
    bool found = false;
    uint32_t i;
    topn_ctr_t *entries = &topn_table[ctr->list_idx];
    topn_ctr_t *entry = entries;

    /* Track replacement candidate */
    topn_ctr_t *replace_candidate = entry;

    /* List empty? */
    if(ctr->list_size == 0) {
        ctr->total = count;
        ctr->list_size = 1;
        entry->value = value;
        entry->count = count;
        return;
    }

    /* Search list for match and determine worst candidate */
    for(i = 0; i < ctr->list_size; i++) {
        if(!found && entry->value == value) {
            entry->count += count;
            ctr->total += count;
            found = true;
        }

        /* Track lowest-count for replacement */
        if (entry->count < replace_candidate->count || 
           (entry->count == replace_candidate->count)) {
            replace_candidate = entry;
        }

        entry++;
    }

    if(found)
        return;

    /* List full? Try to evict worst */
    if(ctr->list_size == TOPN_MAX_SLOT_SIZE) {
        if(count > replace_candidate->count) {
            ctr->total += count - replace_candidate->count;
            replace_candidate->value = value;
            replace_candidate->count = count;
        }
    } 
    else {
        /* Room to add new entry */
        ctr->list_size++;
        ctr->total += count;
        entry->value = value;
        entry->count = count;
    }
}

void topn_write(FILE *f, const gcov_ctr_info_t *ci_ptr) {
    uint32_t i, j, length;
    uint32_t num_pairs = 0;
    uint32_t num_counters = ci_ptr->num / 3;
    topn_counter_t *counters = (topn_counter_t *)ci_ptr->values;

    /* First pass: calculate number of pairs */
    for(i = 0;i < num_counters; i++)
        num_pairs += counters[i].list_size;

    /* Write byte length for all the following data */
    length = (num_counters * 2 + num_pairs * 2) * sizeof(gcov_type);
    fwrite(&length, 1, 4, f);

    for(i = 0;i < num_counters; i++) {
        topn_counter_t *ctr = &counters[i];

        /* Write the total */
        fwrite(&ctr->total, 1, 8, f);

        /* Write the amount of pairs */
        fwrite(&ctr->list_size, 1, 8, f);

        /* Write value-count pairs for this counter */
        topn_ctr_t *entries = &topn_table[counters[i].list_idx];

        for(j = 0;j < counters[i].list_size; j++) {
            fwrite(&entries[j].value, 1, 8, f);
            fwrite(&entries[j].count, 1, 8, f);
        }
    }
}
