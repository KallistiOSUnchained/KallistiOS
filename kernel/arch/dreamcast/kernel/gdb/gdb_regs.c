/* KallistiOS ##version##

   kernel/gdb/gdb_regs.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2025 Andy Barajas

*/

#include <stdlib.h>

#include "gdb_internal.h"

/* Map KOS register context order to GDB sh4 order */
#define KOS_REG(r)      offsetof(irq_context_t, r)

uint32_t kos_reg_map[] = {
    /* General purpose registers (r0-r15) */
    KOS_REG(r[0]), KOS_REG(r[1]), KOS_REG(r[2]), KOS_REG(r[3]),
    KOS_REG(r[4]), KOS_REG(r[5]), KOS_REG(r[6]), KOS_REG(r[7]),
    KOS_REG(r[8]), KOS_REG(r[9]), KOS_REG(r[10]), KOS_REG(r[11]),
    KOS_REG(r[12]), KOS_REG(r[13]), KOS_REG(r[14]), KOS_REG(r[15]),

    /* Control registers */
    KOS_REG(pc), KOS_REG(pr), KOS_REG(gbr), KOS_REG(vbr),
    KOS_REG(mach), KOS_REG(macl), KOS_REG(sr),
    KOS_REG(fpul), KOS_REG(fpscr),

    /* Floating-point registers fr0-fr15 (primary bank) */
    KOS_REG(fr[0]), KOS_REG(fr[1]), KOS_REG(fr[2]), KOS_REG(fr[3]),
    KOS_REG(fr[4]), KOS_REG(fr[5]), KOS_REG(fr[6]), KOS_REG(fr[7]),
    KOS_REG(fr[8]), KOS_REG(fr[9]), KOS_REG(fr[10]), KOS_REG(fr[11]),
    KOS_REG(fr[12]), KOS_REG(fr[13]), KOS_REG(fr[14]), KOS_REG(fr[15]),

    /* "ssr", "spc" */
    KOS_REG(sr), KOS_REG(pc),

    /* Floating-point registers fr16-fr31 (banked registers) */
    KOS_REG(frbank[0]),  KOS_REG(frbank[1]),  KOS_REG(frbank[2]),  KOS_REG(frbank[3]),
    KOS_REG(frbank[4]),  KOS_REG(frbank[5]),  KOS_REG(frbank[6]),  KOS_REG(frbank[7]),
    KOS_REG(frbank[8]),  KOS_REG(frbank[9]),  KOS_REG(frbank[10]), KOS_REG(frbank[11]),
    KOS_REG(frbank[12]), KOS_REG(frbank[13]), KOS_REG(frbank[14]), KOS_REG(frbank[15])
};

#undef KOS_REG

/* Number of registers in above map */
#define REG_COUNT    59

static int32_t gdb_thread_for_regs = -1;

static irq_context_t *regs_irq_ctx;

/* Sets the current thread ID used for register operations. */
void set_regs_thread(int tid) {
    gdb_thread_for_regs = tid;
    setup_regs_context();
}

/* Selects the appropriate IRQ context for the current register thread. */
void setup_regs_context(void) {
    irq_context_t *default_context = gdb_get_irq_context();
    if(gdb_thread_for_regs == GDB_THREAD_ALL || gdb_thread_for_regs == GDB_THREAD_ANY) {
        regs_irq_ctx = default_context; /* Default to current thread context */
        return;
    }

    kthread_t *target = thd_by_tid(gdb_thread_for_regs);

    regs_irq_ctx = (target != NULL)
        ? (irq_context_t *)&target->context
        : default_context;
}

/* Combines two 32-bit floats into a 64-bit double register (drN). */
static __always_inline uint64_t build_dr(uint32_t fr_low, uint32_t fr_high) {
    return ((uint64_t)fr_high << 32) | fr_low;
}

/* Constructs a 128-bit floating vector (fvN) from four FR registers. */
static __always_inline void build_fv(uint32_t fv[4], const uint32_t fr[16], int base) {
    fv[0] = fr[base + 0];
    fv[1] = fr[base + 1];
    fv[2] = fr[base + 2];
    fv[3] = fr[base + 3];
}

/*
   Appends a single register to the GDB T packet in stop reply format.

   Format: nn:vvvvvvvv;
     - nn: register number (2 hex digits)
     - vvvvvvvv: value encoded in hex (endianness-respecting)
     - Ends with a semicolon

   Returns pointer to the end of the output buffer.
*/
static char *append_reg(char *out, int regno, const void *val, size_t size) {
    *out++ = highhex(regno);
    *out++ = lowhex(regno);
    *out++ = ':';
    out = mem_to_hex((const char *)val, out, size);
    *out++ = ';';

    return out;
}

/*
   Appends all SH4 register values to the GDB T packet.

   Includes:
     - General-purpose registers (r0–r15)
     - Control registers (pc, pr, gbr, etc.)
     - Floating-point registers (fr0–fr15, frbank0–15)
     - Pseudo registers (dr0–dr7 as 64-bit, fv0–fv12 as 128-bit)
     - SSR and SPC placeholders if not available
     - Zero-filled stubs for unimplemented registers

   Returns a pointer to the end of the output buffer.
*/
char *append_regs(char *out, irq_context_t *context) {
    /* General-purpose registers r0–r15 */
    for(int i = 0; i < 16; ++i)
        out = append_reg(out, i, &context->r[i], 4);

    /* Control registers */
    out = append_reg(out, 16, &context->pc, 4);
    out = append_reg(out, 17, &context->pr, 4);
    out = append_reg(out, 18, &context->gbr, 4);
    out = append_reg(out, 19, &context->vbr, 4);
    out = append_reg(out, 20, &context->mach, 4);
    out = append_reg(out, 21, &context->macl, 4);
    out = append_reg(out, 22, &context->sr, 4);
    out = append_reg(out, 23, &context->fpul, 4);
    out = append_reg(out, 24, &context->fpscr, 4);

    /* Floating-point registers fr0–fr15 */
    for(int i = 0; i < 16; ++i)
        out = append_reg(out, 25 + i, &context->fr[i], 4);

    /* Expects ssr and spc but we dont have those; send this instead */
    out = append_reg(out, 41, &context->sr, 4);
    out = append_reg(out, 42, &context->pc, 4);

    /* Floating-point registers frbank0–frbank15 */
    for(int i = 0; i < 16; ++i)
        out = append_reg(out, 43 + i, &context->frbank[i], 4);

    return out;
}

/*
   Handle the 'p' command.
   Reads a single register from the target.
   Format: pNN
   Where NN is the register number in hex.
*/
void handle_read_reg(char *ptr) {
    int regnum = strtol(ptr, NULL, 16);
    char *out = gdb_get_out_buffer();

    /* Normal registers in kos_reg_map */
    if(regnum < REG_COUNT) {
        uint32_t *reg_ptr = (uint32_t *)((uintptr_t)regs_irq_ctx + kos_reg_map[regnum]);
        mem_to_hex((char *)reg_ptr, out, 4);
        return;
    }

    /* Skip 9 blank slots */
    if(regnum >= 57 && regnum <= 65) {
        strcpy(out, "00000000");
        return;
    }

    /* Unsupported register */
    gdb_error_with_code_str(GDB_EINVAL, "p: Invalid register %d", regnum);
}

/*
   Handle the 'P' command.
   Writes a single register to the target.
   Format: PNN=rrrrrrrr
   Where NN is the register number in hex and rrrrrrrr is the value in hex.
*/
void handle_write_reg(char *ptr) {
    /* Extract register number */
    int regnum = strtol(ptr, &ptr, 16);

    if(*ptr != '=') {
        gdb_error_with_code_str(GDB_EINVAL, "P: Invalid packet");
        return;
    }

    ptr++; /* skip '=' */

    /* Write to base 32-bit registers */
    if(regnum < REG_COUNT) {
        hex_to_mem(ptr, (char *)((uintptr_t)regs_irq_ctx + kos_reg_map[regnum]), 4);
        gdb_put_ok();
        return;
    }

    /* Ignore dummy 9 registers (regs 57–65) */
    if(regnum >= 57 && regnum <= 65) {
        gdb_put_ok();
        return;
    }

    /* Write to dr0 – dr14 (68–75, even only) */
    if(regnum >= 68 && regnum <= 75 && ((regnum & 1) == 0)) {
        int base = regnum - 68;
        uint64_t dr = 0;
        hex_to_mem(ptr, (char *)&dr, 8);
        regs_irq_ctx->fr[base + 0] = (uint32_t)(dr & 0xFFFFFFFF);
        regs_irq_ctx->fr[base + 1] = (uint32_t)(dr >> 32);
        gdb_put_ok();
        return;
    }

    /* Write to fv0 – fv12 (76–79) */
    if(regnum >= 76 && regnum <= 79) {
        int base = (regnum - 76) * 4;
        uint32_t fv[4];
        hex_to_mem(ptr, (char *)fv, 16);
        for(int i = 0; i < 4; i++)
            regs_irq_ctx->fr[base + i] = fv[i];
        gdb_put_ok();
        return;
    }

    /* Unsupported register */
    gdb_error_with_code_str(GDB_EINVAL, "P: Invalid register %d", regnum);
}

/*
   Handle the 'g' command.
   Returns the full set of general-purpose registers.
*/
void handle_read_regs(char *ptr) {
    (void)ptr;
    int i;

    char *out = gdb_get_out_buffer();
    for(i = 0; i < REG_COUNT; i++) {
        uint32_t *reg_ptr = (uint32_t *)((uintptr_t)regs_irq_ctx + kos_reg_map[i]);
        out = mem_to_hex((char *)reg_ptr, out, 4);
    }

    /* Fill 9 blank registers */
    memset(out, 0, 9*4);
    out += 9*4;

    /* 68–75: dr0, dr2, ..., dr14 (packed pairs of frN) */
    uint64_t dr = 0;
    for(i = 0; i < 16; i += 2) {
        dr = build_dr(regs_irq_ctx->fr[i], regs_irq_ctx->fr[i + 1]);
        out = mem_to_hex((char *)&dr, out, 8);
    }

    /* 76–79: fv0, fv4, fv8, fv12 (packed groups of 4 frN) */
    uint32_t fv[4];
    for(i = 0; i < 16; i += 4) {
        build_fv(fv, regs_irq_ctx->fr, i);
        out = mem_to_hex((char *)fv, out, 16);
    }

    *out = 0;
}

/*
   Handle the 'G' command.
   Writes to all general-purpose registers.
   Format: Gxxxxxxxx.... (entire register state in hex).
*/
void handle_write_regs(char *ptr) {
    int i;
    char *in = ptr;

    for(i = 0; i < REG_COUNT; i++, in += 8)
        hex_to_mem(in, (char *)((uint32_t)regs_irq_ctx + kos_reg_map[i]), 4);

    gdb_put_ok();
}
