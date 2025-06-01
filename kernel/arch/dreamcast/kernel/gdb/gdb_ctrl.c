/* KallistiOS ##version##

   kernel/gdb/gdb_ctrl.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2025 Andy Barajas

*/

/*
   Implements instruction stepping and continue commands for the GDB stub.

   - Supports single-step via software trap patching (trapa #0x20).
   - Handles conditional/unconditional branches, calls, and returns.
   - Supports 'c', 's', 'Cxx', and 'Sxx' remote protocol packets.
   - Allows thread-specific execution control.

   Uses instruction decoding to patch the next instruction and trap on step.
*/

#include <arch/irq.h>
#include <arch/arch.h>
#include <arch/cache.h>

#include "gdb_internal.h"


/* Hitachi SH architecture instruction encoding masks */
#define COND_BR_MASK   0xff00
#define UCOND_DBR_MASK 0xe000
#define UCOND_RBR_MASK 0xf0df
#define TRAPA_MASK     0xff00

#define COND_DISP      0x00ff
#define UCOND_DISP     0x0fff
#define UCOND_REG      0x0f00

/* Hitachi SH instruction opcodes */
#define BF_INSTR       0x8b00
#define BFS_INSTR      0x8f00
#define BT_INSTR       0x8900
#define BTS_INSTR      0x8d00
#define BRA_INSTR      0xa000
#define BSR_INSTR      0xb000
#define JMP_INSTR      0x402b
#define JSR_INSTR      0x400b
#define RTS_INSTR      0x000b
#define RTE_INSTR      0x002b
#define TRAPA_INSTR    0xc300
#define SSTEP_INSTR    0xc320

/* Hitachi SH processor register masks */
#define T_BIT_MASK     0x0001

typedef struct {
    short *mem_addr;
    short old_instr;
} step_data_t;

static bool stepped;
static step_data_t instr_buffer;

static int32_t gdb_thread_for_ctrl = -1;

static irq_context_t *ctrl_irq_ctx;

/* Sets the target thread for control operations (continue/step). */
void set_ctrl_thread(int tid) {
    gdb_thread_for_ctrl = tid;
    setup_ctrl_context();
}

/*
   Sets the IRQ context used for control operations like continue/step.
   If the selected thread is "any" or "all", it defaults to the current context.
*/
void setup_ctrl_context() {
    irq_context_t *default_context = gdb_get_irq_context();
    if(gdb_thread_for_ctrl == GDB_THREAD_ALL || gdb_thread_for_ctrl == GDB_THREAD_ANY) {
        ctrl_irq_ctx = default_context; /* Default to current thread context */
        return;
    }

    kthread_t *target = thd_by_tid(gdb_thread_for_ctrl);
    ctrl_irq_ctx = (target != NULL)
        ? (irq_context_t *)&target->context
        : default_context;
}

/*
   Prepares for single-step execution by patching the next instruction
   with a TRAPA. Calculates the next address based on SH4 branch logic.
*/
void do_single_step(void) {
    short *instr_mem;
    int displacement;
    int reg;
    unsigned short opcode, br_opcode;

    stepped = true;
    instr_mem = (short *) ctrl_irq_ctx->pc;
    opcode = *instr_mem;
    br_opcode = opcode & COND_BR_MASK;

    if(br_opcode == BT_INSTR || br_opcode == BTS_INSTR) {
        if(ctrl_irq_ctx->sr & T_BIT_MASK) {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               Remember PC points to second instr.
               after PC of branch ... so add 4
            */
            instr_mem = (short *)(ctrl_irq_ctx->pc + displacement + 4);
        }
        else {
            /* Can't safely place trapa in BT/S delay slot */
            instr_mem += (br_opcode == BTS_INSTR) ? 2 : 1;
        }
    }
    else if(br_opcode == BF_INSTR || br_opcode == BFS_INSTR) {
        if(ctrl_irq_ctx->sr & T_BIT_MASK) {
            /* Can't put a trapa in the delay slot of a bf/s instruction */
            instr_mem += (br_opcode == BFS_INSTR) ? 2 : 1;
        }
        else {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               Remember PC points to second instr.
               after PC of branch ... so add 4
            */
            instr_mem = (short *)(ctrl_irq_ctx->pc + displacement + 4);
        }
    }
    else if((opcode & UCOND_DBR_MASK) == BRA_INSTR) {
        displacement = (opcode & UCOND_DISP) << 1;

        if(displacement & 0x0800)
            displacement |= 0xfffff000;

        /*
          Remember PC points to second instr.
          after PC of branch ... so add 4
        */
        instr_mem = (short *)(ctrl_irq_ctx->pc + displacement + 4);
    }
    else if((opcode & UCOND_RBR_MASK) == JSR_INSTR) {
        reg = (char)((opcode & UCOND_REG) >> 8);
        instr_mem = (short *) ctrl_irq_ctx->r[reg];
    }
    else if(opcode == RTS_INSTR)
        instr_mem = (short *) ctrl_irq_ctx->pr;
    else if(opcode == RTE_INSTR)
        instr_mem = (short *) ctrl_irq_ctx->r[15];
    else if((opcode & TRAPA_MASK) == TRAPA_INSTR)
        instr_mem = (short *)((opcode & ~TRAPA_MASK) << 2);
    else
        instr_mem += 1;

    instr_buffer.mem_addr = instr_mem;
    instr_buffer.old_instr = *instr_mem;
    *instr_mem = SSTEP_INSTR;
    icache_flush_range((uint32_t)instr_mem, 2);
}

/*
   Undo the effect of a previous do_single_step.  If we single stepped,
   restore the old instruction.
*/
void undo_single_step(void) {
    if(stepped) {
        short *instr_mem;
        instr_mem = instr_buffer.mem_addr;
        *instr_mem = instr_buffer.old_instr;
        icache_flush_range((uint32_t)instr_mem, 2);
    }

    stepped = false;
}

/*
   Handle the 'c' (continue) and 's' (single-step) GDB commands.

   These commands resume execution of the program, optionally from a new PC.
   - 'c' continues execution normally.
   - 's' performs a single instruction step.

   Format:
     - 'c'           → continue from current PC
     - 'cXXXX'       → continue from address XXXX
     - 's'           → single-step from current PC
     - 'sXXXX'       → single-step from address XXXX

   This function parses the optional address (if present) and updates the PC.
   If single-stepping, it prepares the next instruction for trapping.
*/
void handle_continue_step(char *ptr) {
    bool stepping = (ptr[-1] == 's');
    uint32_t addr;

    if(hex_to_int(&ptr, &addr))
        ctrl_irq_ctx->pc = addr;

    if(stepping)
        do_single_step();
}

/*
   Handles the 'C' and 'S' packets for continue or step with signal.

   Format:
     - 'Cxx'         → continue with signal xx
     - 'Sxx'         → step one instruction with signal xx
     - 'Cxx;ADDR'    → continue from address ADDR with signal xx
     - 'Sxx;ADDR'    → step from address ADDR with signal xx

   Signals are ignored on SH4; this just resumes or steps as needed.
   If 'S' is used, single-step mode is enabled before continuing.

   ptr points to the character after 'C' or 'S'.
*/
void handle_continue_step_signal(char *ptr) {
    bool stepping = (ptr[-1] == 'S');
    uint32_t signal = 0;
    uint32_t new_pc = 0;

    /* Parse signal (always two hex digits) */
    if(!hex_to_int(&ptr, &signal)) {
        gdb_error_with_code_str(GDB_EINVAL, "C/S: Invalid packet");
        return;
    }

    /* Optional: skip semicolon and parse new PC if present */
    if(*ptr == ';') {
        ptr++; /* skip ';' */
        if (hex_to_int(&ptr, &new_pc)) {
            ctrl_irq_ctx->pc = new_pc;
        }
    }

    /* Ignore the signal; just continue or step */
    if(stepping)
        do_single_step();

    put_packet(GDB_OK);

    /* If there is no way to recover we must abort */
    arch_abort();

    // Otherwise, exit(0);
}
