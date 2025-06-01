/* KallistiOS ##version##

   kernel/gdb/gdb.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2025 Andy Barajas

*/

#include <stdio.h>

#include <arch/arch.h>

#include <dc/scif.h>
#include <dc/fs_dcload.h>

#include "gdb_internal.h"

/* TRAPA #0x20: internal single-step or resume */
#define TRAPA_GDB_SINGLESTEP     32

/* TRAPA #0x3F: GDB-inserted software breakpoints (Z0) */
#define TRAPA_GDB_BREAKPOINT     63

/* TRAPA #0xFF: manually inserted via gdb_breakpoint() */
#define TRAPA_USER_BREAKPOINT   255

static bool initialized;
static bool connected;
static irq_context_t *current_irq_ctx;

/* Returns the current IRQ context captured during a GDB exception. */
irq_context_t *gdb_get_irq_context(void) {
    return current_irq_ctx;
}

/*
   Main GDB exception handler.

   Called when an exception occurs. This function determines the cause of
   the exception (using the exception vector), notifies GDB of the halt,
   and processes any subsequent remote protocol requests.
*/
void gdb_handle_exception(int exception_vector) {
    char *ptr;

    /* Setup contexts for regs and ctrl handlers */
    setup_regs_context();
    setup_ctrl_context();

    /* Tell GDB why we halted and give additional info */
    handle_t_stop_reply(exception_vector);
    put_out_buffer_packet();

    undo_single_step();

    while(true) {
        gdb_clear_out_buffer();

        /* Receive a command from GDB server */
        ptr = (char *)get_packet();
        connected = true;

        char *orig_command = ptr;
        switch(*ptr++) {
            case '?': handle_t_stop_reply(exception_vector); break;
            case 'D': handle_detach(); break;
            case 'p': handle_read_reg(ptr); break;
            case 'P': handle_write_reg(ptr); break;
            case 'g': handle_read_regs(ptr); break;
            case 'G': handle_write_regs(ptr); break;
            case 'H': handle_thread_select(ptr); break;
            case 'k': handle_kill(); break;
            case 'm': handle_read_mem(ptr); break;
            case 'M': handle_write_mem(ptr); break;
            case 'c':
            case 's':
                handle_continue_step(ptr); return;
            case 'C':
            case 'S':
                handle_continue_step_signal(ptr); break;
            case 'q': handle_query(ptr); break;
            case 'Q': handle_set_query(ptr); break;
            case 'T': handle_thread_alive(ptr); break;
            case 'v': handle_v_packet(ptr); break;
            case 'x': handle_read_mem_binary(ptr); break;
            case 'X': handle_write_mem_binary(ptr); break;
            case 'Z':
            case 'z':
                handle_breakpoint(ptr); break;
            default:
                dbglog(DBG_INFO, "Unhandled GDB command: %s\n", orig_command);
                break;
        }

        put_out_buffer_packet();
    }
}

static void handle_exception(irq_t code, irq_context_t *context, void *data) {
    (void)data;

    current_irq_ctx = context;
    gdb_handle_exception(code);
}

static void handle_user_trapa(trapa_t code, irq_context_t *context, void *data) {
    (void)code;
    (void)data;

    current_irq_ctx = context;
    gdb_handle_exception(EXC_TRAPA);
}

/*
   Handle a TRAPA exception triggered either by a custom call or a GDB-inserted
   software breakpoint (e.g., from a Z0 packet).

   Sets the current IRQ context and delegates to GDB exception handling.
*/
static void handle_gdb_trapa(trapa_t code, irq_context_t *context, void *data) {
    (void)code;
    (void)data;

    current_irq_ctx = context;
    current_irq_ctx->pc -= 2;
    gdb_handle_exception(EXC_TRAPA);
}

/*
   Triggers a software breakpoint using TRAPA #0xFF.

   Typically called at the start of a program to establish a connection
   with the debugger, or used to halt execution and enter the debugger manually.
*/
void gdb_breakpoint(void) {
    __asm__("trapa	#0xff"::);
}

void gdb_shutdown(int status) {
    if(!initialized || !connected)
        return;

    char *out = gdb_get_out_buffer();
    sprintf(out, "W%02x", status & 0xFF);
    put_packet(out);
    arch_abort();

    // Optionally clean up, disconnect, or disable breakpoints
}

void gdb_init(void) {
    if(initialized)
        return;

    initialized = true;
    connected = false;
    if(dcload_gdbpacket(NULL, 0, NULL, 0) == 0)
        set_dcl_mode_enabled(true);
    else
        scif_set_parameters(57600, 1);

    irq_set_handler(EXC_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_SLOT_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_READ, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_WRITE, handle_exception, NULL);
    irq_set_handler(EXC_USER_BREAK_PRE, handle_exception, NULL);

    trapa_set_handler(TRAPA_GDB_SINGLESTEP, handle_gdb_trapa, NULL);
    trapa_set_handler(TRAPA_GDB_BREAKPOINT, handle_gdb_trapa, NULL);
    trapa_set_handler(TRAPA_USER_BREAKPOINT, handle_user_trapa, NULL);

    gdb_breakpoint();
}
