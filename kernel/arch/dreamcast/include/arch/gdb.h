/* KallistiOS ##version##

   arch/dreamcast/include/arch/gdb.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2025 Andy Barajas

*/

/** \file    arch/gdb.h
    \brief   GNU Debugger support.
    \ingroup debugging_gdb

    This file contains functions to set up and utilize GDB with KallistiOS.

    \author Megan Potter
*/

#ifndef __ARCH_GDB_H
#define __ARCH_GDB_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup debugging_gdb GDB
    \brief                  Interface for using the GNU Debugger
    \ingroup                debugging

    @{
*/

/** \brief  Initialize the GDB stub.

    This function initializes GDB support. It should be the first thing you do
    in your program, when you wish to use GDB for debugging.
*/
void gdb_init(void);

/** \brief  Shutdown the GDB stub.

    This function shuts down GDB support. It informs the GDB server that the
    program has exited by sending a packet with the exit status. This is done
    for you by the normal shutdown procedure of KOS. There should not really be
    any reason for you to call this function yourself.

    \param  status  The program exit status to report to GDB.
 */
void gdb_shutdown(int status);

/** \brief  Manually raise a GDB breakpoint.

    This function manually raises a GDB breakpoint at the current location in
    the code, allowing you to inspect things with GDB at the point where the
    function is called.
*/
void gdb_breakpoint(void);

/** @} */

__END_DECLS

#endif  /* __ARCH_GDB_H */

