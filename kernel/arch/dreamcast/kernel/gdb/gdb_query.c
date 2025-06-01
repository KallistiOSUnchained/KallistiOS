/* KallistiOS ##version##

   kernel/gdb/gdb_query.c

   Copyright (C) 2025 Andy Barajas

*/

/*
   Implements GDB remote protocol support for query ('q') and set ('Q') packets.

   Supported features:
     - qSupported           : Advertise supported packets
     - qTStatus             : Report async stop status (no-op)
     - qOffsets             : Report ELF memory section offsets
     - qAttached            : Report if already attached
     - qSymbol              : Reject symbol lookup requests
     - qC                   : Report current thread ID
     - qfThreadInfo / qs... : Report thread IDs
     - qThreadExtraInfo     : Report thread label
     - qGetTLSAddr          : Resolve thread-local storage offset
     - QStartNoAckMode      : Enable no-acknowledgment mode

   All unsupported or malformed queries reply with a standard error message.
*/

#include <stdio.h>

#include "gdb_internal.h"

extern const size_t tcbhead_size;

/*
   Handle the 'T' command.
   Checks whether a thread ID is valid.
   Format: Tnn — where nn is the thread ID in hex.
*/
void handle_thread_alive(char *ptr) {
    uint32_t tid = 0;

    if(hex_to_int(&ptr, &tid)) {
        if(thd_by_tid(tid))
            gdb_put_ok();
        else
            gdb_error_with_code_str(GDB_EINVAL, "T: Thread ID %lu is not valid", tid);
    }
    else {
        gdb_error_with_code_str(GDB_EINVAL, "T: Invalid packet");
    }
}

/* Callback for thd_each() for qfThreadInfo packet. */
static int qfThreadInfo(kthread_t *thd, void *ud) {
    size_t idx = *(size_t*)ud;

    char tid_hex[9]; /* 8 digits + null terminator */
    size_t tid_len = format_thread_id_hex(tid_hex, thd->tid);
    if(idx + tid_len + 1 >= BUFMAX - 1)  /* +1 for comma or terminator */
        return -1;

    char *buffer = gdb_get_out_buffer();
    if(idx > 1)
        buffer[idx++] = ',';

    for(size_t i = 0; i < tid_len; i++)
        buffer[idx++] = tid_hex[i];

    *(size_t*)ud = idx;
    return 0;
}

/*
   Handle the 'Q' command.
   Used for setting various options or state on the target.
   Currently supported:

     QStartNoAckMode — Enable no-acknowledgment mode
     Format: QStartNoAckMode
*/
void handle_set_query(char *ptr) {
    if(strncmp(ptr, "StartNoAckMode", 14) == 0) {
        /* Enable no-ack mode: skip sending and waiting for '+' acks. */
        set_no_ack_mode_enabled(true);
        gdb_put_ok();
    }
    else {
        gdb_error_with_code_str(GDB_EUNIMPL, "Q: Unsupported packet: %.32s", ptr);
    }
}

/* Call this in your qSupported packet handler */
static void parse_qSupported_features(const char *features) {
    set_error_messages_enabled(false);

    const char *p = features;
    while(*p) {
        if(strncmp(p, "error-message+", 14) == 0) {
            set_error_messages_enabled(true);
            break;
        }

        /* Move to the next feature (semicolon separated) */
        const char *next = strchr(p, ';');
        if(!next) break;
        p = next + 1;
    }
}

/*
   Handle the 'q' command.
   Processes various query packets: qC, qfThreadInfo, sThreadInfo, etc.
*/
void handle_query(char *ptr) {
    /* https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#qSupported */
    if(strncmp(ptr, "Supported:", 10) == 0) {
        ptr += 10;
        parse_qSupported_features(ptr);

        char *buffer = gdb_get_out_buffer();
        snprintf(buffer, BUFMAX,
             "PacketSize=%x;"
             "QStartNoAckMode+;"
             "error-message+;"
             "vContSupported+;"
             "xmlRegisters=sh4a;"
             "swbreak+;"
             "hwbreak+;",
             BUFMAX - 4);

        gdb_put_str(buffer);
    }
    /*
       Handle the 'qTStatus' command.
       Reports if there is pending asynchronous stop information.
       Format: qTStatus
       Response: Empty means no pending stop.
    */
    else if(strncmp(ptr, "TStatus", 7) == 0) {
        gdb_clear_out_buffer();
    }
    /*
       Handle the 'qOffsets' command.
       Requests the memory offsets for text, data, and bss.
       Format: qOffsets
       Response: text=ADDR;data=ADDR;bss=ADDR
    */
    else if(strncmp(ptr, "Offsets", 7) == 0) {
        gdb_put_str("Text=0;Data=0;Bss=0");
    }
    /*
       Handle the 'qAttached' command.
       Reports if the debugger was already attached (1) or newly attached (0).
       Format: qAttached
       Response: 0 or 1
    */
    else if(strncmp(ptr, "Attached", 8) == 0) {
        gdb_put_str("1");
    }
    /*
       Handle the 'qSymbol' command.
       GDB sends this to initiate or respond to symbol lookups.

       Format:
         GDB → target:
           qSymbol::                 → GDB is ready to provide symbol values
           qSymbol:<value>:<sym>     → response to a prior request (value can be empty)

         Target → GDB:
           qSymbol:<hex_sym_name>    → request value of a symbol (e.g., "main")
           OK                        → done requesting symbols

       GDB will keep replying with qSymbol:<value>:<name> until stub replies "OK".
    */
    else if(strncmp(ptr, "Symbol", 6) == 0) {
        ptr += 6;

        if(ptr[0] == ':' && ptr[1] == ':') {
            /* Initial handshake from GDB: qSymbol:: */
            /* We tell GDB we dont want any */
            gdb_put_ok();
        }
    }
    /*
       Handle the 'qC' command.
       Reports the current active thread ID.
       Format: qC
       Response: QCNN — where NN is the thread ID in hex (2 hex digits).
    */
    else if(*ptr == 'C') {
        kthread_t *thd = thd_get_current();
        char *buffer = gdb_get_out_buffer();
        strcpy(buffer, "QC");
        format_thread_id_hex(buffer + 2, thd->tid);
    }
    /*
       Handle the 'qfThreadInfo' command.
       Format: qfThreadInfo
       Response: mNNNNNNNN,...
    */
    else if(strncmp(ptr, "fThreadInfo", 11) == 0) {
        size_t idx = 0;
        char *buffer = gdb_get_out_buffer();
        buffer[idx++] = 'm';
        thd_each(qfThreadInfo, &idx);
        buffer[idx] = '\0';
    }
    /*
       Handle the 'qsThreadInfo' command.
       Returns continuation thread list data after a 'qfThreadInfo' packet.
       Format: qsThreadInfo

       GDB calls 'qfThreadInfo' to fetch the first chunk of thread IDs,
       and continues calling 'qsThreadInfo' to get additional thread IDs
       until the stub replies with 'l' (lowercase L), indicating the end.

       If all thread IDs fit in the first 'qfThreadInfo' reply,
       you can return 'l' here immediately to indicate completion.
    */
    else if(strncmp(ptr, "sThreadInfo", 11) == 0) {
        gdb_put_str("l");
    }
    /*
       Handle the 'qThreadExtraInfo' command.
       Provides human-readable information about a specific thread.
       Format: qThreadExtraInfo,TT
       Where TT is the thread ID in hex.

       The stub should respond with the thread's label or description,
       encoded as a hex string. GDB may display this in thread lists.

       Example:
         Request:  qThreadExtraInfo,04
         Response: 6D61696E20746872656164   ("main thread")
    */
    else if(strncmp(ptr, "ThreadExtraInfo,", 16) == 0) {
        ptr += 16;
        uint32_t tid = 0;
        if(hex_to_int(&ptr, &tid)) {
            kthread_t *thr = thd_by_tid(tid);
            if(thr) {
                const char *label = thd_get_label(thr);
                if(label) {
                    char *buffer = gdb_get_out_buffer();
                    mem_to_hex(label, buffer, strlen(label));
                }
                else
                    gdb_clear_out_buffer();
            }
            else
                gdb_error_with_code_str(GDB_EINVAL, "qThreadExtraInfo: No thread with TID=%lu", tid);
        }
        else
            gdb_error_with_code_str(GDB_EINVAL, "qThreadExtraInfo: Invalid packet");
    }
    /*
       Handle the 'qGetTLSAddr' command.
       Returns the address of a TLS variable for a specific thread.
       Format: qGetTLSAddr:TID,OFFSET,LMID
        - TID: Thread ID
        - OFFSET: Offset from the base of TLS block
        - LMID: Link map ID (ignored in KOS)
       Response: hex address of TLS variable
    */
    else if(strncmp(ptr, "GetTLSAddr:", 11) == 0) {
        ptr += 11;
        uint32_t tid = 0, offset = 0, lm = 0;
        if(hex_to_int(&ptr, &tid) && *ptr++ == ',' &&
           hex_to_int(&ptr, &offset) && *ptr++ == ',' &&
           hex_to_int(&ptr, &lm)) {
            kthread_t *thd = thd_by_tid(tid);
            if(thd && thd->tls_hnd) {
                char *buffer = gdb_get_out_buffer();
                uintptr_t tls_base = (uintptr_t)thd->tls_hnd + tcbhead_size + offset;
                mem_to_hex((char *)&tls_base, buffer, sizeof(tls_base));
                // void *tls_addr = (void *)((uintptr_t)thd->tls_hnd + tcbhead_size + offset);
                // mem_to_hex((char *)&tls_addr, buffer, sizeof(tls_addr));
            }
            else {
                gdb_error_with_code_str(GDB_EINVAL, "qGetTLSAddr: TLS base unavailable");
            }
        }
        else {
            gdb_error_with_code_str(GDB_EINVAL, "qGetTLSAddr: Invalid packet");
        }
    }
}
