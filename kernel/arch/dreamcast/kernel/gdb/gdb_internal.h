#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <kos/thread.h>
#include <kos/dbglog.h>

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers. At least NUMREGBYTES*2 are needed for register packets.
 */
#define BUFMAX         1024

#define GDB_OK         "OK"

/* Generic errors */
#define GDB_ERROR_BAD_ARGUMENTS            "E06"
#define GDB_ERROR_UNSUPPORTED_COMMAND      "E07"
/* Memory and register errors */
#define GDB_ERROR_MEMORY_BAD_ADDRESS       "E30"
#define GDB_ERROR_MEMORY_BUS_ERROR         "E31"
#define GDB_ERROR_MEMORY_TIMEOUT           "E32"
#define GDB_ERROR_MEMORY_VERIFY_ERROR      "E33"
#define GDB_ERROR_MEMORY_BAD_ACCESS_SIZE   "E34"
#define GDB_ERROR_MEMORY_GENERAL           "E35"
/* Breakpoint errors */
#define GDB_ERROR_BKPT_NOT_SET             "E50" /* Unable to set breakpoint */
#define GDB_ERROR_BKPT_SWBREAK_NOT_SET     "E51" /* Unable to write software breakpoint to memory */
#define GDB_ERROR_BKPT_HWBREAK_NO_RSRC     "E52" /* No hardware breakpoint resource available to set hardware breakpoint */
#define GDB_ERROR_BKPT_HWBREAK_ACCESS_ERR  "E53" /* Failed to access hardware breakpoint resource */
#define GDB_ERROR_BKPT_CLEARING_BAD_ID     "E55" /* Bad ID when clearing breakpoint */
#define GDB_ERROR_BKPT_CLEARING_BAD_ADDR   "E56" /* Bad address when clearing breakpoint */
#define GDB_ERROR_BKPT_SBREAKER_NO_RSRC    "E57" /* Insufficient hardware resources for software breakpoints */

char highhex(int x);
char lowhex(int x);
int hex(char ch);

char *mem_to_hex(const char *src, char *dest, size_t count);
char *hex_to_mem(const char *src, char *dest, size_t count);
size_t hex_to_int(char **ptr, uint32_t *int_value);
void undo_single_step(void);

extern bool stepping;
extern bool using_dcl;
extern char remcom_out_buffer[];
extern irq_context_t *irq_ctx;

void build_error_packet(const char *fmt, ...);
unsigned char *get_packet(void);
void put_packet(const char *buffer);

void handle_read_regs(char *ptr);
void handle_write_regs(char *ptr);
void handle_read_mem(char *ptr);
void handle_write_mem(char *ptr);
void handle_continue_step(char *ptr);
void handle_breakpoint(char *ptr);
void handle_query(char *ptr);
void handle_thread_alive(char *ptr);
