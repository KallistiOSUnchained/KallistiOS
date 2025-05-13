/* KallistiOS ##version##

   kernel/gdb/gdb.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2025 Andy Barajas

*/

/* Remote communication protocol.

   A debug packet whose contents are <data>
   is encapsulated for transmission in the form:

    $ <data> # CSUM1 CSUM2

    <data> must be ASCII alphanumeric and cannot include characters
    '$' or '#'.  If <data> starts with two characters followed by
    ':', then the existing stubs interpret this as a sequence number.

    CSUM1 and CSUM2 are ascii hex representation of an 8-bit
    checksum of <data>, the most significant nibble is sent first.
    the hex digits 0-9,a-f are used.

   Receiver responds with:

    +   - if CSUM is correct and ready for next packet
    -   - if CSUM is incorrect

   <data> is as follows:
   All values are encoded in ascii hex digits.
**********************************************************************
    REQUEST     PACKET
**********************************************************************
    read regs   g
    
    reply       XX....X     Each byte of register data
                            is described by two hex digits.
                            Registers are in the internal order
                            for GDB, and the bytes in a register
                            are in the same order the machine uses.
                or ENN      for an error.
**********************************************************************
    write regs  GXX..XX     Each byte of register data
                            is described by two hex digits.
    
    reply       OK          for success
                ENN         for an error
**********************************************************************
    write reg   Pn...=r...  Write register n... with value r...,
                            which contains two hex digits for each
                            byte in the register (target byte
                            order).
    
    reply       OK          for success
                ENN         for an error
    (not supported by all stubs).
**********************************************************************
    read mem    mAA..AA,LLLL    
                            AA..AA is address, LLLL is length.
    reply       XX..XX      XX..XX is mem contents

                            Can be fewer bytes than requested
                            if able to read only part of the data.
                or ENN      NN is errno
**********************************************************************
    write mem   MAA..AA,LLLL:XX..XX
                            AA..AA is address,
                            LLLL is number of bytes,
                            XX..XX is data

    reply       OK          for success
                ENN         for an error (this includes the case
                            where only part of the data was
                            written).
**********************************************************************
    cont        cAA..AA     AA..AA is address to resume
                            If AA..AA is omitted,
                            resume at same address.
**********************************************************************
    step        sAA..AA     AA..AA is address to resume
                            If AA..AA is omitted,
                            resume at same address.
**********************************************************************
    last signal ?           Reply the current reason for stopping.
                            This is the same reply as is generated
                            for step or cont : SAA where AA is the
                            signal number.

    There is no immediate reply to step or cont.
    The reply comes when the machine stops.
    It is       SAA     AA is the "signal number"

    or...       TAAn...:r...;n:r...;n...:r...;
                            AA = signal number
                            n... = register number
                            r... = register contents
    or...       WAA         The process exited, and AA is
                            the exit status.  This is only
                            applicable for certains sorts of
                            targets.
**********************************************************************

**********************************************************************
    reserved    <other>     On other requests, the stub should
                            ignore the request and send an empty
                            response ($#<checksum>).  This way
                            we can extend the protocol and GDB
                            can tell whether the stub it is
                            talking to uses the old or the new.
**********************************************************************
    search      tAA:PP,MM   Search backwards starting at address
                            AA for a match with pattern PP and
                            mask MM.  PP and MM are 4 bytes.
                            Not supported by all stubs.
**********************************************************************
    general query   
                qXXXX       Request info about XXXX.
**********************************************************************
    general set QXXXX=yyyy  Set value of XXXX to yyyy.
**********************************************************************
    query sect offs 
                qOffsets    Get section offsets.  
    reply       Text=xxx;Data=yyy;Bss=zzz
**********************************************************************
    console output  
                Otext       Send text to stdout.  Only comes from
                            remote target.
**********************************************************************
    set command-line args
                Aarglen,argnum,arg,…
                            Initialized `argv[]' array passed into 
                            program. arglen specifies the number of 
                            bytes in the hex encoded byte stream arg. 
                            See `gdbserver' for more details.

    reply       OK 
    reply       ENN
**********************************************************************
    Current thread 
                qC      Return the current thread ID

    reply       QCpid   pid is HEX encoded 16-bit process iD
    reply       *       implies old process ID

**********************************************************************

    Responses can be run-length encoded to save space.  A '*' means that
    the next character is an ASCII encoding giving a repeat count which
    stands for that many repetitions of the character preceding the '*'.
    The encoding is n+29, yielding a printable character where n >=3
    (which is where rle starts to win).  Don't use an n > 126.

    So
    "0* " means the same as "0000".  */

#include <arch/arch.h>

#include <dc/scif.h>
#include <dc/fs_dcload.h>

#include "gdb_internal.h"

static bool initialized;

irq_context_t *irq_ctx;
int last_sigval;

/*
 * this function takes the SH-1 exception number and attempts to
 * translate this number into a unix compatible signal value
 */
static int compute_signal(int exception_vector) {
    int sigval;

    switch(exception_vector) {
        case EXC_ILLEGAL_INSTR:
        case EXC_SLOT_ILLEGAL_INSTR:
            sigval = 4;
            break;
        case EXC_DATA_ADDRESS_READ:
        case EXC_DATA_ADDRESS_WRITE:
            sigval = 10;
            break;

        case EXC_TRAPA:
            sigval = 5;
            break;

        default:
            sigval = 7;       /* "software generated"*/
            break;
    }

    return sigval;
}

/*
 * Handle the '?' command.
 * Responds with the signal that caused the target to stop.
 * Format: Sxx where xx is the hex signal number.
 * TODO: TXX
 */
static void handle_status() {
    remcom_out_buffer[0] = 'S';
    remcom_out_buffer[1] = highhex(last_sigval);
    remcom_out_buffer[2] = lowhex(last_sigval);
    remcom_out_buffer[3] = 0;
}

/*
    This function does all exception handling.  It only does two things -
    it figures out why it was called and tells gdb, and then it reacts
    to gdb's requests.
*/
extern const size_t tcbhead_size;
static void gdb_handle_exception(int exception_vector) {
    char *ptr;

    last_sigval = compute_signal(exception_vector);
    handle_status();

    put_packet(remcom_out_buffer);

    undo_single_step();

    while(true) {
        remcom_out_buffer[0] = 0;
        ptr = (char *)get_packet();

        switch(*ptr++) {
            case '?': handle_status(); break;
            case 'g': handle_read_regs(ptr); break;
            case 'G': handle_write_regs(ptr); break;
            case 'm': handle_read_mem(ptr); break;
            case 'M': handle_write_mem(ptr); break;
            case 'c': 
            case 's': 
                handle_continue_step(ptr); return;
            case 'Z': 
            case 'z': 
                handle_breakpoint(ptr); break;
            case 'q': handle_query(ptr); break;
            case 'T': handle_thread_alive(ptr); break;
            case 'k': arch_abort(); break;
            default:
                dbglog(DBG_INFO, "Unhandled GDB command: %s\n", ptr);
                break;
        }

        put_packet(remcom_out_buffer);
    }
}


static void handle_exception(irq_t code, irq_context_t *context, void *data) {
    (void)data;

    irq_ctx = context;
    gdb_handle_exception(code);
}

static void handle_user_trapa(trapa_t code, irq_context_t *context, void *data) {
    (void)code;
    (void)data;

    irq_ctx = context;
    gdb_handle_exception(EXC_TRAPA);
}

static void handle_gdb_trapa(trapa_t code, irq_context_t *context, void *data) {
    /*
    * trapa 0x20 indicates a software trap inserted in
    * place of code ... so back up PC by one
    * instruction, since this instruction will
    * later be replaced by its original one!
    */
    (void)code;
    (void)data;

    irq_ctx = context;
    irq_ctx->pc -= 2;
    gdb_handle_exception(EXC_TRAPA);
}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */

void gdb_breakpoint(void) {
    __asm__("trapa	#0xff"::);
}

void gdb_init(void) {
    if(initialized)
        return;

    initialized = true;
    if(dcload_gdbpacket(NULL, 0, NULL, 0) == 0)
        using_dcl = true;
    else
        scif_set_parameters(57600, 1);

    irq_set_handler(EXC_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_SLOT_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_READ, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_WRITE, handle_exception, NULL);
    irq_set_handler(EXC_USER_BREAK_PRE, handle_exception, NULL);

    trapa_set_handler(32, handle_gdb_trapa, NULL);
    trapa_set_handler(255, handle_user_trapa, NULL);

    gdb_breakpoint();
}
