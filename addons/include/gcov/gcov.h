/* KallistiOS ##version##

   gcov/gcov.h
   Copyright (C) 2025 Andy Barajas
*/

/** \file      gcov/gcov.h
    \brief     Minimal GCOV runtime implementation.
    \ingroup   gcov

    This file defines the public interface for the GCOV profiling runtime. 
    It enables GCC's `--coverage` and `-fprofile-generate` functionality. The 
    implementation is compatible with GCC 13+ and supports generation and manual 
    dumping of `.gcda` coverage files, including full support for all standard 
    counter types.

    \author Andy Barajas
*/

#ifndef __GCOV_GCOV_H
#define __GCOV_GCOV_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup gcov  GCOV
    \brief          Lightweight GCOV profiling runtime for KOS
    \ingroup        debugging

    This file provides runtime support for GCOV, a coverage analysis tool built
    into GCC. GCOV allows you to determine which parts of your code were executed,
    how often, and which branches were taken. This is especially helpful for
    analyzing test coverage or identifying unused code paths.

    Supported Profiling Modes:

    1. `-fprofile-generate`:

        Enables profile data generation. This inserts instrumentation into your
        code to collect execution counts for functions, branches, and arcs during
        runtime. The results are written to `.gcda` files when `__gcov_exit()` or
        `__gcov_dump()` is called.

        Example:
        ```sh
        CFLAGS += -fprofile-generate
        ```

    2. `-fprofile-use`:

        Recompiles your code using previously collected `.gcda` files (from
        `-fprofile-generate`) to guide optimizations. This can improve performance
        by reordering code based on actual runtime behavior.

        Example:
        ```sh
        CFLAGS += -fprofile-use
        ```

    3. `-ftest-coverage` (or `--coverage`):

        Enables both `-fprofile-arcs` and `-ftest-coverage`, which insert
        instrumentation and generate `.gcno` metadata files during compilation.
        These `.gcno` files are required to interpret `.gcda` data later.

        Example:
        ```sh
        CFLAGS += --coverage
        ```

    Collecting and Analyzing Coverage:

    1. **Compile with coverage support:**

        ```sh
        CFLAGS += --coverage
        ```

        This generates `.gcno` files alongside your object files.

    2. **Run your program on Dreamcast (with `-fprofile-generate`):**

        Coverage data will be collected in memory during execution. You can manually
        trigger a dump at any point by calling:

        ```c
        __gcov_dump();  // or __gcov_exit();
        ```

        This writes `.gcda` files to the filesystem, redirected to `/pc` by default.

    3. **Generate an HTML report with LCOV:**

        Use this Makefile target to capture and visualize results:

        ```make
        lcov:
        	lcov \
        		--gcov-tool=/opt/toolchains/dc/sh-elf/bin/sh-elf-gcov \
        		--directory . \
        		--base-directory . \
        		--capture \
        		--output-file coverage.info
        	genhtml coverage.info --output-directory html
        	open html/index.html
        ```

        This generates a full HTML report with annotated source code in the `html/` directory.

    \author Andy Barajas

    @{
*/

/** \brief Environment variable to set the output directory for `.gcda` files.
    \ingroup gcov

    If set, this value is prepended to the stripped source path to form the
    final output path.
*/
#define GCOV_PREFIX        "GCOV_PREFIX"

/** \brief Environment variable to control path stripping.
    \ingroup gcov

    Specifies how many leading directory components should be removed from the
    source file path before generating the `.gcda` output file.
*/
#define GCOV_PREFIX_STRIP  "GCOV_PREFIX_STRIP"

/** \brief Clears all collected runtime coverage counters.
    \ingroup gcov

    This function resets all counters in memory without writing them out.
    Useful for restarting coverage collection mid-run.
*/
void __gcov_reset(void);

/** \brief Writes all registered coverage data to `.gcda` files.
    \ingroup gcov

    This function flushes all registered coverage counters to disk using the
    current GCOV output rules. Called automatically on exit, but can also be
    called manually for intermediate coverage snapshots.
*/
void __gcov_dump(void);

/** @} */

__END_DECLS

#endif /* !__GCOV_GCOV_H */
