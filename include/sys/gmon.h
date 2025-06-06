/* KallistiOS ##version##

   sys/gmon.h
   Copyright (C) 2025 Andy Barajas

*/

#ifndef __SYS_GMON_H
#define __SYS_GMON_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stdbool.h>

/** \defgroup gprof GPROF
    \brief          Lightweight GPROF profiling runtime for KOS
    \ingroup        debugging


    This file provides utilities for profiling applications using gprof. Gprof
    is a profiling tool that allows you to analyze where your program spent 
    its time and which functions called which other functions during execution. 
    This can help you identify performance bottlenecks and optimize your code.

    Profiling Steps:

    1. Compile your program with profiling flags:
    
        Use the following flags:
        ```sh
        CFLAGS = -pg -fno-omit-frame-pointer -fno-inline
        ```
        These flags enable profiling and prevent function inlining to ensure 
        accurate profiling data.  When you use the -pg flag, the GCC compiler 
        inserts trapa #33 instructions at the beginning of each function into 
        your build.

    2. Running your program to create gmon.out:

        Execute your program normally to completion using dcload-ip or 
        dcload-serial. This will generate a file named `gmon.out` in the current 
        directory, which contains the profiling data.

    3. Running gprof:

        Use the following command to analyze the profiling data:
        ```sh
        $(KOS_GPROF) $(TARGET) gmon.out > gprof_output.txt
        ```
        Replace `$(TARGET)` with the name of your compiled program. This 
        command will generate a human-readable report in `gprof_output.txt`.

    4. Visualizing profiling data with gprof2dot:

        To create a graphical representation of the profiling data, use the 
        `gprof2dot` tool:
        ```sh
        gprof2dot gprof_output.txt > graph.dot
        ```
        This converts the `gprof` output into a DOT format file, which can be 
        used to generate various types of graphs.

    5. Generating an image from the DOT file:

        Use the `dot` command from Graphviz to create a SVG image from the DOT 
        file:
        ```sh
        dot -Tsvg graph.dot -o graph.svg
        ```
        This command will generate a SVG image (`graph.svg`) that visually 
        represents the profiling data.

    By following these steps, you can effectively profile your program and 
    identify performance bottlenecks for optimization.

    \author Andy Barajas

    @{
*/

/** \brief  GPROF Trapa Code

    GCC generates this pattern before each profiled function when you compile 
    your project with the -pg flag:

    trapa #33 --- This is a 2-byte instruction
    nop       --- This is a 2-byte no-op placeholder
    label     --- This is a 4-byte LABELNO
*/
#define GPROF_TRAPA_CODE  33

/** \brief  Start gprof profiling for a specified address range.

    This function is intended for programs not linked with the `-pg' linker 
    switch. If `-pg' was used during linking, monstartup is automatically 
    called by the startup code with arguments covering the entire range of 
    executable addresses, from the program's entry point to the highest code 
    segment address. Using this method to initialize gprof will only generate 
    histogram profiling data, without producing a call graph. Profiling starts 
    immediately after calling this function.

    \param  lowpc   The lower bound of the address range to profile.
    \param  highpc  The upper bound of the address range to profile.
*/
void monstartup(uintptr_t lowpc, uintptr_t highpc);

/** \brief  Restart or stop gprof profiling.

    This function restarts or stops gprof profiling. It does NOT start gprof 
    profiling initially, as gprof profiling starts before the program enters 
    the main function. You can use this function to stop profiling and then 
    restart it later when you reach the section of code you want to profile.

    \param  enable            A boolean value to restart (true) or stop (false) 
                              gprof profiling.
*/
void moncontrol(bool enable);

/** @} */

__END_DECLS

#endif  /* __SYS_GMON_H */
