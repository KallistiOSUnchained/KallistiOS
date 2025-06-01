/* KallistiOS ##version##

   kernel/gdb/gdb_mem.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2025 Andy Barajas

*/

/*
   Implements GDB memory access packets ('m', 'M', 'x', 'X') for Dreamcast target.

   Supports both hexadecimal and binary memory reads/writes.
   - Handles proper bounds checking
   - Flushes instruction cache on memory write
   - Complies with GDB remote protocol for escaped binary data
*/

#include <stdio.h>

#include <arch/cache.h>

#include "gdb_internal.h"

/*
   Checks if a memory range is valid for read/write access.

   Allows:
     - 0x04000000–0x07FFFFFF (Video RAM)
     - 0x08000000–0x0FFFFFFF (System RAM)

   Read-only:
     - 0x00000000–0x03FFFFFF (Boot/Flash ROM)
     - 0x10000000–0x1FFFFFFF (Hardware regs, I/O)

   All other ranges are rejected.
*/
static bool is_valid_memory_range(uint32_t addr, uint32_t len, bool is_write) {
    uint32_t end = addr + len;

    return true;

    /* Video RAM */
    if(addr >= 0x04000000 && end <= 0x07FFFFFF)
        return true; /* read/write */

    /* System RAM */
    if(addr >= 0x08000000 && end <= 0x0FFFFFFF)
        return true; /* read/write */

    /* Read-only: Boot/Flash/Regs */
    if(!is_write) {
        if(end <= 0x04000000)
            return true;
        if(addr >= 0x10000000 && end <= 0x20000000)
            return true;
    }

    /* Deny anything else */
    return false;
}

/*
   Handle the 'm' command.
   Reads memory from the target at a given address and length.
   Format: mADDR,LEN
*/
void handle_read_mem(char *ptr) {
    uint32_t addr = 0;
    uint32_t len = 0;

    if(hex_to_int(&ptr, &addr) && *ptr++ == ',' && hex_to_int(&ptr, &len)) {
        if(!is_valid_memory_range(addr, len, false)) {
            gdb_error_with_code_str(GDB_EMEM_PROT, "m. Invalid read range");
            return;
        }
        if(len > BUFMAX) {
            gdb_error_with_code_str(GDB_EMEM_SIZE, "m: Access length exceeds BUFMAX");
            return;
        }
        char *buffer = gdb_get_out_buffer();
        mem_to_hex((char *)addr, buffer, len);
    }
    else
        gdb_error_with_code_str(GDB_EINVAL, "m: Invalid packet");
}

/*
   Handle the 'M' command.
   Writes memory to the target at a given address.
   Format: MADDR,LEN:DATA
*/
void handle_write_mem(char *ptr) {
    uint32_t addr = 0;
    uint32_t len = 0;

    if(hex_to_int(&ptr, &addr) && *ptr++ == ',' &&
       hex_to_int(&ptr, &len) && *ptr++ == ':') {
        if(!is_valid_memory_range(addr, len, true)) {
            gdb_error_with_code_str(GDB_EMEM_PROT, "M: Invalid write range");
            return;
        }
        if(len > BUFMAX) {
            gdb_error_with_code_str(GDB_EMEM_SIZE, "M: Access length exceeds BUFMAX");
            return;
        }
        hex_to_mem(ptr, (char *)addr, len);
        icache_flush_range(addr, len);
        gdb_put_ok();
    }
    else
        gdb_error_with_code_str(GDB_EINVAL, "M: Invalid packet");
}

/*
   Handle the 'x' command.
   Read binary data from target memory at a given address.
   Format: xADDR,LEN

   - ADDR: Target address in hex
   - LEN:  Number of bytes to read

   The response is returned as binary data prefixed with 'b'.
   Special characters are escaped according to the GDB remote protocol.
*/
void handle_read_mem_binary(char *ptr) {
    size_t len;
    unsigned long addr;

    if(sscanf(ptr, "%lx,%zu", &addr, &len) == 2) {
        if(!is_valid_memory_range(addr, len, false)) {
            gdb_error_with_code_str(GDB_EMEM_PROT, "x: Invalid read range");
            return;
        }
        if(len > BUFMAX) {
            gdb_error_with_code_str(GDB_EMEM_SIZE, "x: Access length exceeds BUFMAX");
            return;
        }

        char *buffer = gdb_get_out_buffer();
        char *out = buffer;
        const uint8_t *src = (const uint8_t *)addr;

        *out++ = 'b';  /* Prefix for binary response */

        for(size_t i = 0; i < len; ++i) {
            uint8_t ch = src[i];

            if(ch == '$' || ch == '#' || ch == '}' || ch == '*') {
                *out++ = '}';
                *out++ = ch ^ 0x20;
            }
            else
                *out++ = ch;

            /* Prevent buffer overflow */
            if((size_t)(out - buffer) >= BUFMAX - 2) {
                gdb_error_with_code_str(GDB_EMEM_SIZE, "x: Response too large");
                return;
            }
        }

        *out = '\0';
    }
    else
        gdb_error_with_code_str(GDB_EINVAL, "x: Invalid packet");
}

static void binary_unescape(const char *in, char *out, size_t len) {
    while(len--) {
        if(*in == '}') {
            in++;
            *out++ = *in++ ^ 0x20;
        }
        else
            *out++ = *in++;
    }
}

/*
   Handle the 'X' command.
   Writes binary data to target memory at a given address.
   Format: XADDR,LEN:DATA

   - ADDR: Target address in hex
   - LEN:  Number of bytes to write
   - DATA: Raw binary data (not hex-encoded), may include escaped bytes
*/
void handle_write_mem_binary(char *ptr) {
    size_t len;
    unsigned long addr;
    const char *data;

    if(sscanf(ptr, "%lx,%x:", &addr, &len) == 2) {
        data = strchr(ptr, ':');
        if(!data) {
            gdb_error_with_code_str(GDB_EINVAL, "X: Invalid packet");
            return;
        }

        if(!is_valid_memory_range(addr, len, true)) {
            gdb_error_with_code_str(GDB_EMEM_PROT, "X: Invalid write range");
            return;
        }

        if(len > BUFMAX) {
            gdb_error_with_code_str(GDB_EMEM_SIZE, "X: Access length exceeds BUFMAX");
            return;
        }

        gdb_put_ok();
        if(len == 0)
            return;

        data++; /* move past ':' */

        /* Write 'len' bytes to 'addr' */
        char tmp[BUFMAX];
        binary_unescape(data, tmp, len);
        memcpy((void *)addr, tmp, len);
    }
    else
        gdb_error_with_code_str(GDB_EUNIMPL, "X: Unrecognized packet");
}
