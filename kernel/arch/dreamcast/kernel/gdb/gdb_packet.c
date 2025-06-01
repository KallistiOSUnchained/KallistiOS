/* KallistiOS ##version##

   kernel/gdb/gdb_packet.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2025 Andy Barajas

*/

/*
   Implements the core transport layer of the GDB remote serial protocol.

   This file handles the low-level I/O for reading and writing GDB packets, including
   checksum validation, optional run-length encoding, and DCLoad/SCIF backend switching.

   Supported features:
    - Full GDB packet parsing with checksum validation
    - Run-length encoding for packet compression
    - Optional no-acknowledgement mode (`QStartNoAckMode`)
    - Dual transport support: SCIF or DCLoad (detected automatically)
*/

#include <stdio.h>
#include <stdarg.h>

#include <dc/scif.h>
#include <dc/fs_dcload.h>

#include "gdb_internal.h"

static bool using_dcl = false;
static bool no_ack_mode = false;
static bool error_messages = false;
static char in_dcl_buf[BUFMAX];
static char out_dcl_buf[BUFMAX];
static uint32_t in_dcl_pos = 0;
static uint32_t out_dcl_pos = 0;
static uint32_t in_dcl_size = 0;
static char remcom_in_buffer[BUFMAX];
static char remcom_out_buffer[BUFMAX];

/* Returns a pointer to the GDB output buffer. */
char *gdb_get_out_buffer(void) {
    return remcom_out_buffer;
}

/* Clears the GDB output buffer by setting the first byte to null. */
void gdb_clear_out_buffer(void) {
    remcom_out_buffer[0] = '\0';
}

/* Writes an "OK" response to the GDB output buffer. */
void gdb_put_ok(void) {
    strcpy(remcom_out_buffer, GDB_OK);
}

/* Writes a custom string response to the GDB output buffer. */
void gdb_put_str(const char *msg) {
    strcpy(remcom_out_buffer, msg);
}

/* Enable or disable DCL (dc-load) I/O mode. */
void set_dcl_mode_enabled(bool enabled) {
    using_dcl = enabled;
}

/* Enable or disable support for textual GDB error messages (E.msg). */
void set_error_messages_enabled(bool enabled) {
    error_messages = enabled;
}

/* Enable or disable no-acknowledgment mode for GDB remote protocol. */
void set_no_ack_mode_enabled(bool enabled) {
    no_ack_mode = enabled;
}

/*
   Sends an error message as a human-readable console message (via GDB 'E.msg' packet)
   or a machine-readable error code (e.g. `E01`, `E02`, etc.).
*/
void gdb_error_with_code_str(const char *errcode, const char *msg_fmt, ...) {
    char msg[BUFMAX];
    va_list args;

    /* Format the human-readable message */
    va_start(args, msg_fmt);
    vsnprintf(msg, sizeof(msg), msg_fmt, args);
    va_end(args);

    if(error_messages) {
        /* GDB supports error text: send E.msg */
        snprintf(remcom_out_buffer, BUFMAX, "E.%s", msg);
    }
    else {
        /* Fallback to E##: return 2-digit numeric code */
        strncpy(remcom_out_buffer, errcode, BUFMAX - 1);
        remcom_out_buffer[BUFMAX - 1] = '\0';
    }
}

/*
   Reads one byte from the debug channel.
   Uses DCLoad or SCIF depending on context.
   Blocks until data is available.
*/
static char get_debug_char(void) {
    int ch;

    if(using_dcl) {
        if(in_dcl_pos >= in_dcl_size) {
            in_dcl_size = dcload_gdbpacket(NULL, 0, in_dcl_buf, BUFMAX);
            in_dcl_pos = 0;
        }

        ch = in_dcl_buf[in_dcl_pos++];
    }
    else {
        /* Spin while nothing is available. */
        while((ch = scif_read()) < 0);

        ch &= 0xff;
    }

    return ch;
}

/*
   Sends a single character over the debug channel.
   Buffered when using DCLoad; flushed immediately on SCIF.
*/
static void put_debug_char(char ch) {
    if(using_dcl) {
        out_dcl_buf[out_dcl_pos++] = ch;

        if(out_dcl_pos >= BUFMAX) {
            dcload_gdbpacket(out_dcl_buf, out_dcl_pos, NULL, 0);
            out_dcl_pos = 0;
        }
    }
    else {
        /* write the char and flush it. */
        scif_write(ch);
        scif_flush();
    }
}

/*
   Flushes the output buffer to the host.
   With DCLoad, handles combined input/output exchange if needed.
*/
static void flush_debug_channel(void) {
    /* send the current complete packet and wait for a response */
    if(using_dcl) {
        if(in_dcl_pos >= in_dcl_size) {
            in_dcl_size = dcload_gdbpacket(out_dcl_buf, out_dcl_pos, in_dcl_buf, BUFMAX);
            in_dcl_pos = 0;
        }
        else
            dcload_gdbpacket(out_dcl_buf, out_dcl_pos, NULL, 0);

        out_dcl_pos = 0;
    }
}

/*
   Reads a full GDB packet from the debug channel.
   Format: $<data>#<checksum>
   Automatically verifies checksum and handles acknowledgement.
*/
unsigned char *get_packet(void) {
    unsigned char *buffer = (unsigned char *)remcom_in_buffer;
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    while(true) {
        /* Wait around for the start character, ignore all other characters */
        while((ch = get_debug_char()) != '$')
            ;

    retry:
        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /* Now, read until a # or end of buffer is found */
        while(count < (BUFMAX-1)) {
            ch = get_debug_char();

            if(ch == '$')
                goto retry;

            if(ch == '#')
                break;

            checksum = checksum + ch;
            buffer[count] = ch;
            count = count + 1;
        }

        buffer[count] = 0;

        if(ch == '#') {
            ch = get_debug_char();
            xmitcsum = hex(ch) << 4;
            ch = get_debug_char();
            xmitcsum += hex(ch);

            if(checksum != xmitcsum) {
                if(!no_ack_mode) put_debug_char('-');
            }
            else {
                if(!no_ack_mode) put_debug_char('+');

                if(buffer[2] == ':') {
                    put_debug_char(buffer[0]);
                    put_debug_char(buffer[1]);
                    return &buffer[3];
                }

                return &buffer[0];
            }
        }
    }
}

/*
   Sends a GDB response packet using optional run-length encoding.
   Format: $<data>#<checksum>
   Retransmits until the host sends an ACK (unless no-ack mode is active).
*/
void put_packet(const char *buffer) {
    int check_sum;

    /*  $<packet info>#<checksum>. */
    do {
        const char *src = buffer;
        put_debug_char('$');
        check_sum = 0;

        while(*src) {
            int runlen;

            /* Do run length encoding */
            for(runlen = 0; runlen < 100; runlen ++) {
                if(src[0] != src[runlen] || runlen == 99) {
                    if(runlen > 3) {
                        int encode;
                        /* Got a useful amount */
                        put_debug_char(*src);
                        check_sum += *src;
                        put_debug_char('*');
                        check_sum += '*';
                        check_sum += (encode = runlen + ' ' - 4);
                        put_debug_char(encode);
                        src += runlen;
                    }
                    else {
                        put_debug_char(*src);
                        check_sum += *src;
                        src++;
                    }

                    break;
                }
            }
        }

        put_debug_char('#');
        put_debug_char(highhex(check_sum));
        put_debug_char(lowhex(check_sum));
        flush_debug_channel();

        if(no_ack_mode)
            break;
    } while(get_debug_char() != '+');
}

void put_out_buffer_packet(void) {
    put_packet(remcom_out_buffer);
}
