/* KallistiOS ##version##

   kernel/gdb/gdb_break.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2025 Andy Barajas

*/

/*
   Implements software and hardware breakpoints for GDB remote protocol.

   Supported:
     - Z0/z0: Software breakpoints (via instruction patching)
     - Z1–Z4/z1–z4: Hardware breakpoints and watchpoints (via SH4 UBC)

   Uses:
     - `trapa #0x3F` as the software breakpoint opcode
     - UBC controller for two simultaneous hardware breakpoints

   Restrictions:
     - Software breakpoints must be 2-byte aligned
     - Hardware breakpoints limited to 8-byte regions
*/

#include <dc/ubc.h>
#include <arch/cache.h>

#include "gdb_internal.h"

#define GDB_BRK_SW    0
#define GDB_BRK_HW    1
#define GDB_WATCH_W   2
#define GDB_WATCH_R   3
#define GDB_WATCH_RW  4

#define MAX_SW_BREAKPOINTS 32

#define GDB_SW_BREAK_OPCODE  0xC33F

typedef struct {
    uint32_t addr;
    uint16_t original;
    int active;
} sw_breakpoint_t;

/*
   Insert or remove a software breakpoint.
   Replaces instruction at 2-byte aligned address with TRAPA.
*/
static sw_breakpoint_t sw_bkpts[MAX_SW_BREAKPOINTS];
static void soft_breakpoint(bool set, uintptr_t addr, size_t length, char* res_buffer) {
    if((addr & 1) || length != 2) {
        gdb_error_with_code_str(GDB_EINVAL, "soft_breakpoint: address not 2-byte aligned or length != 2");
        return;
    }

    if(set) {
        for(int i = 0; i < MAX_SW_BREAKPOINTS; i++) {
            if(!sw_bkpts[i].active) {
                sw_bkpts[i].addr = addr;
                sw_bkpts[i].original = *((uint16_t *)addr);
                sw_bkpts[i].active = 1;
                *((uint16_t *)addr) = GDB_SW_BREAK_OPCODE;
                icache_flush_range(addr, 2);
                strcpy(res_buffer, GDB_OK);
                return;
            }
        }
        gdb_error_with_code_str(GDB_EBKPT_SET_FAIL, "soft_breakpoint: no available slot");
    }
    else {
        for(int i = 0; i < MAX_SW_BREAKPOINTS; i++) {
            if(sw_bkpts[i].active && sw_bkpts[i].addr == addr) {
                *((uint16_t *)addr) = sw_bkpts[i].original;
                icache_flush_range(addr, 2);
                sw_bkpts[i].active = 0;
                strcpy(res_buffer, GDB_OK);
                return;
            }
        }
        gdb_error_with_code_str(GDB_EBKPT_CLEAR_ADDR,
            "soft_breakpoint: no active breakpoint at address %p", (void *)addr);
    }
}

/*
   Sets or clears a hardware breakpoint/watchpoint using SH4 UBC.
   Supports instruction, read, write, or access break types.
   Uses one of two UBC slots; fails on large size or no slot.
*/
static ubc_breakpoint_t gdb_hw_bps[2];
static void hard_breakpoint(bool set, int brk_type, uintptr_t addr, size_t length, char *res_buffer) {
    ubc_breakpoint_t *slot = NULL;
    int i;

    /* GDB tries to watch 0, wasting a UCB channel */
    if(addr == 0) {
        strcpy(res_buffer, GDB_OK);
        return;
    }

    /* Can't match large lengths */
    if(length > 8) {
        gdb_error_with_code_str(GDB_EBKPT_SW_OVERWRITE, "hard_breakpoint: length > 8 bytes unsupported");
        return;
    }

    /* Find slot */
    for(i = 0; i < 2; ++i) {
        if(!gdb_hw_bps[i].address || gdb_hw_bps[i].address == (void *)addr) {
            slot = &gdb_hw_bps[i];
            break;
        }
    }

    if(!slot) {
        gdb_error_with_code_str(GDB_EBKPT_HW_NORES, "hard_breakpoint: no free UBC slots");
        return;
    }

    if(set) {
        memset(slot, 0, sizeof(*slot));
        slot->address = (void *)addr;

        switch(brk_type) {
            case GDB_BRK_HW: /* Instruction */
                slot->access = ubc_access_instruction;
                slot->instruction.break_before = true;
                break;
            case GDB_WATCH_W: /* Write */
                slot->access = ubc_access_operand;
                slot->operand.rw = ubc_rw_write;
                slot->operand.size = ubc_size_any;
                break;
            case GDB_WATCH_R: /* Read */
                slot->access = ubc_access_operand;
                slot->operand.rw = ubc_rw_read;
                slot->operand.size = ubc_size_any;
                break;
            case GDB_WATCH_RW: /* Access (rw) */
                slot->access = ubc_access_operand;
                slot->operand.rw = ubc_rw_either;
                slot->operand.size = ubc_size_any;
                break;
            default:
                gdb_error_with_code_str(GDB_EINVAL, "hard_breakpoint: invalid breakpoint type");
                return;
        }

        if(!ubc_add_breakpoint(slot, NULL, NULL)) {
            memset(slot, 0, sizeof(*slot));
            gdb_error_with_code_str(GDB_EBKPT_SET_FAIL, "hard_breakpoint: ubc_add_breakpoint() failed");
            return;
        }

        strcpy(res_buffer, GDB_OK);
    }
    else {
        if(!slot->address) {
            gdb_error_with_code_str(GDB_EBKPT_CLEAR_ADDR, "hard_breakpoint: no active breakpoint at address");
            return;
        }

        if(ubc_remove_breakpoint(slot)) {
            memset(slot, 0, sizeof(*slot));
            strcpy(res_buffer, GDB_OK);
        }
        else
            gdb_error_with_code_str(GDB_EBKPT_CLEAR_ID, "hard_breakpoint: failed to clear breakpoint");
    }
}

/*
   Handle the 'Z' and 'z' commands.
   Inserts or removes a breakpoint or watchpoint.
   Format: Ztype,addr,kind or ztype,addr,kind
*/
void handle_breakpoint(char *ptr) {
    bool set = (ptr[-1] == 'Z');
    int brk_type = *ptr++ - '0';
    uint32_t addr;
    uint32_t length;

    if(*ptr++ == ',' && hex_to_int(&ptr, &addr) &&
       *ptr++ == ',' && hex_to_int(&ptr, &length)) {

        if(brk_type < GDB_BRK_SW || brk_type > GDB_WATCH_RW) {
            gdb_error_with_code_str(GDB_EINVAL, "handle_breakpoint: invalid breakpoint type");
            return;
        }

        char *buffer = gdb_get_out_buffer();
        if(brk_type == GDB_BRK_SW)
            soft_breakpoint(set, addr, length, buffer);
        else
            hard_breakpoint(set, brk_type, addr, length, buffer);
    }
    else {
        gdb_error_with_code_str(GDB_EINVAL, "handle_breakpoint: malformed Z/z packet");
    }
}


// /* Handle inserting/removing a hardware breakpoint.
//    Using the SH4 User Break Controller (UBC) we can have
//    two breakpoints, each set for either instruction and/or operand access.
//    Break channel B can match a specific data being moved, but there is
//    no GDB remote protocol spec for utilizing this functionality. */

// #define LREG(r, o) (*((uint32_t*)((r)+(o))))
// #define WREG(r, o) (*((uint16_t*)((r)+(o))))
// #define BREG(r, o) (*((uint8_t*)((r)+(o))))

// static void hard_breakpoint(bool set, int brk_type, uintptr_t addr, size_t length, char* res_buffer) {
//     char* const ucb_base = (char*)0xff200000;
//     static const int ucb_step = 0xc;
//     static const char BAR = 0x0, BAMR = 0x4, BBR = 0x8, /*BASR = 0x14,*/ BRCR = 0x20;

//     static const uint8_t bbrBrk[] = {
//         0x0,  /* type 0, memory breakpoint -- unsupported */
//         0x14, /* type 1, hardware breakpoint */
//         0x28, /* type 2, write watchpoint */
//         0x24, /* type 3, read watchpoint */
//         0x2c  /* type 4, access watchpoint */
//     };

//     uint8_t bbr = 0;
//     char* ucb;
//     int i;

//     if(length <= 8) {
//         do {
//             bbr++;
//         } while(length >>= 1);
//     }

//     bbr |= bbrBrk[brk_type];

//     if(addr == 0) {
//         strcpy(res_buffer, GDB_OK);
//     }
//     else if(brk_type == 0) {
//         /* we don't support memory breakpoints -- the debugger
//            will use the manual memory modification method */
//         *res_buffer = '\0';
//     }
//     else if(length > 8) {
//         strcpy(res_buffer, GDB_EBKPT_SW_OVERWRITE);
//     }
//     else if(set) {
//         WREG(ucb_base, BRCR) = 0;

//         /* find a free UCB channel */
//         for(ucb = ucb_base, i = 2; i > 0; ucb += ucb_step, i--)
//             if(WREG(ucb, BBR) == 0)
//                 break;

//         if(i) {
//             LREG(ucb, BAR) = addr;
//             BREG(ucb, BAMR) = 0x4; /* no BASR bits used, all BAR bits used */
//             WREG(ucb, BBR) = bbr;
//             strcpy(res_buffer, GDB_OK);
//         }
//         else
//             strcpy(res_buffer, GDB_EBKPT_SET_FAIL);
//     }
//     else {
//         /* find matching UCB channel */
//         for(ucb = ucb_base, i = 2; i > 0; ucb += ucb_step, i--)
//             if(LREG(ucb, BAR) == addr && WREG(ucb, BBR) == bbr)
//                 break;

//         if(i) {
//             WREG(ucb, BBR) = 0;
//             strcpy(res_buffer, GDB_OK);
//         }
//         else
//             strcpy(res_buffer, "E06");
//     }
// }

// #undef LREG
// #undef WREG
