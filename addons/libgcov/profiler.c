/* KallistiOS ##version##

    profiler.c
    Copyright (C) 2025 Andy Barajas

*/

#include <kos/mutex.h>
#include <kos/dbglog.h>

#include <gcov/gcov.h>
#include "gcov.h"

/* Required when compiling with -fprofile-generate for time and indirect call profiling. */
gcov_type __gcov_time_profiler_counter = 0;
_Thread_local indirect_call_t __gcov_indirect_call;

/*
    Tracks the frequency of indirect function calls (e.g., virtual calls or function pointers).

    - Before a call, instrumentation stores the expected callee in `__gcov_indirect_call.callee`
      and the associated counters in `__gcov_indirect_call.counters`.
    - This function is called after the indirect call executes.
    - If the actual callee matches the expected one, it records the callee_id in the TopN set.

    Use case:
        Enables profile-guided devirtualization and indirect call promotion.
        GCC can then optimize common indirect calls as direct branches.
*/
void __gcov_indirect_call_profiler_v4(gcov_type callee_id, void *callee) {
    if(callee == __gcov_indirect_call.callee && __gcov_indirect_call.ctr) {
        topn_add_value(__gcov_indirect_call.ctr, callee_id, 1);
    }
    __gcov_indirect_call.callee = NULL;
}

/*
    Tracks the most frequently observed values at runtime.

    - Called when a profiled site (e.g., a switch case) executes with a particular value.
    - Adds the value to the corresponding TopN counter set.

    Use case:
        Helps compilers optimize for hot values, like common branches or loop bounds.
*/
void __gcov_topn_values_profiler(topn_counter_t *counter, gcov_type value) {
    topn_add_value(counter, value, 1);
}

/*
    Tracks the average of a series of values.
    - counters[0] stores the running sum
    - counters[1] stores the number of samples
 */
void __gcov_average_profiler(gcov_type *counters, gcov_type value) {
    counters[0] += value;
    counters[1] += 1;
}

/*
    Tracks which interval (bucket) a given value falls into.
    - Useful for histograms (e.g., loop trip counts, size classes).
    - Values < start increment underflow bucket: counters[steps + 1]
    - Values ≥ start + steps increment overflow bucket: counters[steps]
    - Values in range [start, start + steps - 1] go to their corresponding bucket
*/
void __gcov_interval_profiler(gcov_type *counters, gcov_type value, int start, uint32_t steps) {
    gcov_type delta = value - start;

    if(delta < 0)
        counters[steps + 1] += 1;  /* Underflow */
    else if(delta >= steps)
        counters[steps] += 1;      /* Overflow */
    else
        counters[delta] += 1;      /* Valid bucket */
}

/*
    Tracks whether a given value is a power of two.
    - counters[0] counts non-power-of-2 values
    - counters[1] counts power-of-2 values

    Useful in analyzing whether runtime values are power-of-two aligned,
    which has implications for bitmasking, fast division, and vectorization.
*/
void __gcov_pow2_profiler(gcov_type *counters, gcov_type value) {
    if(value == 0 || (value & (value - 1)) != 0)
        counters[0] += 1; /* Not a power of 2 */
    else
        counters[1] += 1; /* Power of 2 */
}

/*
    Tracks a bitwise OR of all values seen.
    - counters[0] |= value for every call
*/
void __gcov_ior_profiler(gcov_type *counters, gcov_type value) {
    counters[0] |= value;
}
