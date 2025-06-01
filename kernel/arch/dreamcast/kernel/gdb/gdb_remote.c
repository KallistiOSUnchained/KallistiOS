/* KallistiOS ##version##

   kernel/gdb/gdb_remote.c

   Copyright (C) 2025 Andy Barajas

*/

#include <stdio.h>

#include <arch/arch.h>

#include "gdb_internal.h"

#define SIGILL  4   /* Illegal Instruction */
#define SIGTRAP 5   /* Breakpoint Trap */
#define SIGBUS  7   /* Bus Error */
#define SIGSEGV 11  /* Segmentation Fault */

static int last_sigval;

/*
   Translates SH-4 exception vector numbers into Unix-compatible signal values
   for use with GDB. This mapping allows the debugger to interpret Dreamcast
   exceptions using standard Unix signals.
 */
static void compute_signal(int exception_vector) {
    switch(exception_vector) {
        case EXC_ILLEGAL_INSTR:
        case EXC_SLOT_ILLEGAL_INSTR:
            last_sigval = SIGILL;
            break;
        case EXC_DATA_ADDRESS_READ:
        case EXC_DATA_ADDRESS_WRITE:
            last_sigval = SIGSEGV;
            break;

        case EXC_TRAPA:
            last_sigval = SIGTRAP;
            break;

        default:
            last_sigval = SIGBUS;
            break;
    }
}

/*
   Handle the 'D' (detach) command.
   Instructs the stub to detach from the target and resume execution.
   Format: D

   GDB expects the stub to reply with "OK" and allow the program to continue
   running normally without debugger intervention.
*/
void handle_detach() {
    put_packet(GDB_OK);
    arch_abort();
}

/*
   Handle the 'k' (kill) command.
   Instructs the stub to terminate the program being debugged.
   Format: k

   GDB expects the stub to acknowledge with "OK" and shut down execution.
*/
void handle_kill() {
    put_packet(GDB_OK);
    arch_abort();
}

/*
   Constructs a `T` stop reply packet to notify GDB that the target has halted.
   Includes the signal, register values, thread ID, and stop reason.

   Format:
     Txx{reg}:{val};...thread:{tid};reason:{r};

   The stop reason is derived from the exception vector (e.g., swbreak, hwbreak).
*/
void handle_t_stop_reply(int exception_vector) {
    compute_signal(exception_vector);
    char *out = gdb_get_out_buffer();

    *out++ = 'T';
    *out++ = highhex(last_sigval);
    *out++ = lowhex(last_sigval);

    out = append_regs(out, gdb_get_irq_context());

    kthread_t *thd = thd_get_current();
    strncpy(out, "thread:", strlen("thread:"));
    out += strlen("thread:");
    out += format_thread_id_hex(out, thd->tid);
    *out++ = ';';

    /* Add reason field based on exception vector */
    const char *reason = NULL;
    switch(exception_vector) {
        case EXC_TRAPA:
            reason = "swbreak"; break;
        case EXC_USER_BREAK_PRE: /* or EXC_USER_BREAK_POST */
            reason = "hwbreak"; break;
        case EXC_DATA_ADDRESS_WRITE:
            reason = "watch"; break;
        case EXC_DATA_ADDRESS_READ:
            reason = "rwatch"; break;
        default:
            reason = "signal"; break;
    }

    strcpy(out, "reason:");
    out += strlen("reason:");
    strcpy(out, reason);
    out += strlen(reason);
    *out++ = ';';

    *out = 0;
}

/*
   Handle the 'H' packet to select the active thread for GDB operations.

   Format:
     - HgXX → Set thread for register ops (g, G, p, P)
     - HcXX → Set thread for control ops (c, s)

   XX is a thread ID in hex. Special values:
     - 0        → Any thread (default)
     - 0xFFFFFFFF (or -1) → All threads

   Updates internal thread tracking so future ops target the correct context.
*/
void handle_thread_select(char *ptr) {
    char type = *ptr++; /* 'g' or 'c' */
    uint32_t tid;

    if(*ptr == '-') {
        tid = -1;
        ptr++;
    }
    else if(!hex_to_int(&ptr, &tid)) {
        gdb_error_with_code_str(GDB_EINVAL, "H: No thread ID");
        return;
    }

    if(type == 'g')
        set_regs_thread(tid);
    else if(type == 'c')
        set_ctrl_thread(tid);
    else {
        gdb_error_with_code_str(GDB_EBADCMD, "H: Unrecognized command %c", type);
        return;
    }

    gdb_put_ok();
}
