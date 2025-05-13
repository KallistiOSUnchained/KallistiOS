/* KallistiOS ##version##

   kernel/gdb/gdb_thread.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2025 Andy Barajas

*/

#include "gdb_internal.h"

extern const size_t tcbhead_size;

/*
 * Handle the 'T' command.
 * Checks whether a thread ID is valid.
 * Format: Tnn — where nn is the thread ID in hex.
 */
void handle_thread_alive(char *ptr) {
    uint32_t tid = 0;
    if(hex_to_int(&ptr, &tid)) {
        if(thd_by_tid(tid))
            strcpy(remcom_out_buffer, GDB_OK);
        else
            build_error_packet("Thread ID %lu is not valid", tid);
    } else {
        build_error_packet("Malformed thread-alive packet");
    }
}

/* Callback for thd_each() for qfThreadInfo packet. */
static int qfThreadInfo(kthread_t *thd, void *ud) {
    size_t idx = *(size_t*)ud;

    if(idx >= BUFMAX - 3)
        return -1;
    if(idx > 1)
        remcom_out_buffer[idx++] = ',';

    remcom_out_buffer[idx++] = highhex(thd->tid);
    remcom_out_buffer[idx++] = lowhex(thd->tid);

    //printf("qfThreadInfo: %u", thd->tid);

    *(size_t*)ud = idx;

    return 0;
}

/*
 * Handle the 'q' command.
 * Processes various query packets: qC, qfThreadInfo, sThreadInfo, etc.
 */
void handle_query(char *ptr) {
    if(*ptr == 'C') {
        kthread_t *thd = thd_get_current();
        remcom_out_buffer[0] = 'Q';
        remcom_out_buffer[1] = 'C';
        remcom_out_buffer[2] = highhex(thd->tid);
        remcom_out_buffer[3] = lowhex(thd->tid);
        remcom_out_buffer[4] = '\0';
    }
    else if(strncmp(ptr, "fThreadInfo", 11) == 0) {
        size_t idx = 0;
        remcom_out_buffer[idx++] = 'm';
        thd_each(qfThreadInfo, &idx);
        remcom_out_buffer[idx] = '\0';
    }
    else if(strncmp(ptr, "sThreadInfo", 11) == 0) {
        strcpy(remcom_out_buffer, "l");
    }
    else if(strncmp(ptr, "ThreadExtraInfo,", 16) == 0) {
        ptr += 16;
        uint32_t tid = 0;
        if(hex_to_int(&ptr, &tid)) {
            kthread_t *thr = thd_by_tid(tid);
            const char *label = thd_get_label(thr);
            mem_to_hex(label, remcom_out_buffer, strlen(label));
        } else {
            build_error_packet("Invalid ThreadExtraInfo query");
        }
    }
    else if(strncmp(ptr, "GetTLSAddr:", 11) == 0) {
        ptr += 11;
        uint32_t tid = 0, offset = 0, lm = 0;
        if(hex_to_int(&ptr, &tid) && *ptr++ == ',' &&
           hex_to_int(&ptr, &offset) && *ptr++ == ',' &&
           hex_to_int(&ptr, &lm)) {
            kthread_t *thd = thd_by_tid(tid);
            if(thd && thd->tls_hnd) {
                void *tls_addr = (void *)((uintptr_t)thd->tls_hnd + tcbhead_size + offset);
                mem_to_hex((char *)&tls_addr, remcom_out_buffer, sizeof(tls_addr));
            } else {
                build_error_packet("TLS base unavailable");
            }
        } else {
            build_error_packet("Bad GetTLSAddr format");
        }
    }
}