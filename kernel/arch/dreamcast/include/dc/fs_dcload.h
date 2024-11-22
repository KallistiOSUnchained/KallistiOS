/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/fs_dcload.h
   Copyright (C) 2002 Andrew Kieschnick

*/

/** \file    dc/fs_dcload.h
    \brief   Implementation of dcload "filesystem".
    \ingroup vfs_dcload

    This file contains declarations related to using dcload, both in its -ip and
    -serial forms. This is only used for dcload-ip support if the internal
    network stack is not initialized at start via KOS_INIT_FLAGS().

    \author Andrew Kieschnick
    \see    dc/fs_dclsocket.h
*/

#ifndef __DC_FS_DCLOAD_H
#define __DC_FS_DCLOAD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>
#include <kos/dbgio.h>

/** \defgroup vfs_dcload    PC
    \brief                  VFS driver for accessing a remote PC via
                            DC-Load/Tool
    \ingroup                vfs

    @{
*/

/* \cond */
extern dbgio_handler_t dbgio_dcload;
/* \endcond */

/* dcload magic value */
/** \brief  The dcload magic value! */
#define DCLOADMAGICVALUE    0xdeadbeef

/** \brief  The address of the dcload magic value */
#define DCLOADMAGICADDR     (uint32_t *)0x8c004004

/* Are we using dc-load-serial or dc-load-ip? */
#define DCLOAD_TYPE_NONE    -1      /**< \brief No dcload connection */
#define DCLOAD_TYPE_SER     0       /**< \brief dcload-serial connection */
#define DCLOAD_TYPE_IP      1       /**< \brief dcload-ip connection */

/** \brief  What type of dcload connection do we have? */
extern int dcload_type;

/* \cond */
/* Available dcload console commands */
#define DCLOAD_READ         0
#define DCLOAD_WRITE        1
#define DCLOAD_OPEN         2
#define DCLOAD_CLOSE        3
#define DCLOAD_CREAT        4
#define DCLOAD_LINK         5
#define DCLOAD_UNLINK       6
#define DCLOAD_CHDIR        7
#define DCLOAD_CHMOD        8
#define DCLOAD_LSEEK        9
#define DCLOAD_FSTAT        10
#define DCLOAD_TIME         11
#define DCLOAD_STAT         12
#define DCLOAD_UTIME        13
#define DCLOAD_ASSIGNWRKMEM 14
#define DCLOAD_EXIT         15
#define DCLOAD_OPENDIR      16
#define DCLOAD_CLOSEDIR     17
#define DCLOAD_READDIR      18
#define DCLOAD_GETHOSTINFO  19
#define DCLOAD_GDBPACKET    20

/* dcload syscall function */
int dcloadsyscall(uint32_t syscall, ...);

/* dcload dirent */
struct dcload_dirent {
    int       d_ino;        /* inode number */
    off_t     d_off;        /* offset to the next dirent */
    uint16_t  d_reclen;     /* length of this record */
    uint8_t   d_type;       /* type of file */
    char      d_name[256];  /* filename */
};

typedef struct dcload_dirent dcload_dirent_t;

/* dcload stat */
typedef struct dcload_stat {
    uint16_t st_dev;
    uint16_t st_ino;
    int st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint16_t st_rdev;
    int st_size;
    int atime;
    int st_spare1;
    int mtime;
    int st_spare2;
    int ctime;
    int st_spare3;
    int st_blksize;
    int st_blocks;
    int st_spare4[2];
} dcload_stat_t;

/* Printk replacement */
void dcload_printk(const char *str);

/* GDB tunnel */
size_t dcload_gdbpacket(const char *in_buf, size_t in_size, char *out_buf, size_t out_size);

/* Init func */
void fs_dcload_init_console(void);
void fs_dcload_init(void);
void fs_dcload_shutdown(void);

/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_FS_DCLOAD_H */
