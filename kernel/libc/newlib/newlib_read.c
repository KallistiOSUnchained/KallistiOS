/* KallistiOS ##version##

    newlib_read.c
    Copyright (C) 2004 Megan Potter
    Copyright (C) 2024 Andress Barajas
*/

#include <kos/fs.h>
#include <kos/dbgio.h>

#include <unistd.h>

long _read_r(void *reent, int fd, void *buf, size_t cnt) {
    (void)reent;
    if(fd == STDIN_FILENO)
        return dbgio_read_buffer((uint8 *)buf, cnt);

    return fs_read(fd, buf, cnt);
}
