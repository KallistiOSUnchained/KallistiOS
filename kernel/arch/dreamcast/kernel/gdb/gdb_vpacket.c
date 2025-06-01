/* KallistiOS ##version##

   kernel/gdb/gdb_vpacket.c

   Copyright (C) 2025 Andy Barajas

*/

#include <arch/arch.h>

#include "gdb_internal.h"

/*
   Processes GDB 'vCont' subcommands.
   Currently supports:
     - 'vCont;c' → continue
     - 'vCont;s' → single-step
*/
static void handle_vcont_action(const char *args) {
    /* Support only 'c' (continue) and 's' (single step) */
    if(strncmp(args, "c", 1) == 0 || strncmp(args, "s", 1) == 0) {
        char cmd_wrapper[2] = { args[0], '\0' };
        handle_continue_step(cmd_wrapper + 1);  /* ptr[-1] will be 'c' or 's' */
    }
    else {
        gdb_error_with_code_str(GDB_EUNIMPL, "vCont: Unsupported vCont action");
    }
}

/*
   Handle GDB 'v' packets for extended operations.

   Supported subcommands:
     - vCont?         → Report supported actions ('c' and 's')
     - vCont;c        → Continue execution
     - vCont;s        → Step one instruction
     - vMustReplyEmpty → Acknowledge empty reply support
     - vCtrlC         → Soft reboot (Ctrl+C triggered)
     - vKill          → Abort execution
*/
void handle_v_packet(char *ptr) {
    if(strncmp(ptr, "Cont?", 5) == 0) {
        strcpy(gdb_get_out_buffer(), "vCont;c;s");
    }
    else if(strncmp(ptr, "Cont;", 5) == 0) {
       handle_vcont_action(ptr + 5);
    }
    else if(strncmp(ptr, "MustReplyEmpty", 14) == 0) {
        gdb_clear_out_buffer();
    }
    else if(strncmp(ptr, "CtrlC", 5) == 0) {
        put_packet(GDB_OK);
        arch_reboot();
    }
    else if(strncmp(ptr, "Kill", 4) == 0) {
        put_packet(GDB_OK);
        arch_abort();
    }
}
