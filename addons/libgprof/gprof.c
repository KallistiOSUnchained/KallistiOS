/* KallistiOS ##version##

    gprof.c
    Copyright (C) 2024 Andy Barajas

    This file contains functions that provide a workaround to get gprof
    working on our current setup without editing startup.s and dealing with
    trapa instructions. This implementation uses `-finstrument-functions` 
    instead of `-pg` and requires linking against `-lgprof`.

    Note: This implementation requires the following compilation and linking flags:

    CFLAGS = -g -finstrument-functions
    LDFLAGS = -lgprof
*/

#include <stdint.h>

/* Start and End address for .text portion of program */
#define BASE_ADDRESS 0x8c010000
extern char _etext;

/* Forward declarations for gprof related functions in libc/koslib/gmon.c */
extern void _mcount(uintptr_t frompc, uintptr_t selfpc);
extern void __monstartup(uintptr_t lowpc, uintptr_t highpc);
extern void _mcleanup(void);

/* Profiling function called at the entry of each function */
void __attribute__ ((no_instrument_function, hot)) __cyg_profile_func_enter(void *this, void *callsite) {
    _mcount((uintptr_t)callsite, (uintptr_t)this);
}

/* Profiling function called at the exit of each function */
void __attribute__ ((no_instrument_function, hot)) __cyg_profile_func_exit(void *this, void *callsite) {
    (void)this;
    (void)callsite;
    
    /* Do nothing */
}

/* Constructor function to initialize profiling. Executed before main() */
void __attribute__ ((no_instrument_function, constructor)) main_constructor(void) {
    __monstartup(BASE_ADDRESS, (uintptr_t)&_etext);
}

/* Destructor function to clean up profiling. Executed after return from main() */
void __attribute__ ((no_instrument_function, destructor)) main_destructor(void) {
    _mcleanup();
}