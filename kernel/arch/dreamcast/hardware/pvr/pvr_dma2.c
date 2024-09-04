/* KallistiOS ##version##

   pvr_dma2.c

   Copyright (C) 2024 Andy Barajas

 */

#include <stdio.h>
#include <errno.h>
#include <dc/pvr.h>
#include <dc/asic.h>
#include <dc/dmac.h>
#include <dc/sq.h>
#include <kos/thread.h>
#include <kos/sem.h>

#include "pvr_internal.h"

typedef struct {
    uintptr_t     pvr_addr;       /* PVR start address */
    uintptr_t     sh4_addr;       /* SH-4 start address */
    uint32_t      size;           /* Size in bytes; Must be a multiple of 32 */
    uint32_t      dir;            /* 0: sh4->PVR; 1: PVR->sh4 */
    uint32_t      trigger_select; /* DMA trigger select; 0-CPU, 1-HW */
    uint32_t      enable;         /* DMA enable */
    uint32_t      start;          /* DMA start */
} pvr_dma_ctrl_t;

/* DMA registers */
#define PVR_DMA_REG_BASE 0xa05f7c00
static volatile pvr_dma_ctrl_t * const pvr_dma = (pvr_dma_ctrl_t *)PVR_DMA_REG_BASE;
static vuint32 *pvr_dma_pro = (vuint32 *)0xa05f7c80;

#define CPU_TRIGGER       0
#define HARDWARE_TRIGGER  1

/* Protection register code. */
#define PVR_DMA_UNLOCK_CODE      0x6702
/* All PVR memory protection values. */
#define PVR_DMA_UNLOCK_ALLMEM    (PVR_DMA_UNLOCK_CODE << 16 | 0x007F)
#define PVR_DMA_LOCK_ALLMEM      (PVR_DMA_UNLOCK_CODE << 16 | 0x7F00)

static semaphore_t pvr_dma_done;
static int32_t pvr_dma_blocking;
static pvr_dma_callback_t pvr_dma_callback;
static void *pvr_dma_cbdata;

static void pvr_dma_irq_hnd(uint32_t code, void *data) {
    (void)code;
    (void)data;

    /* Call the callback, if any. */
    if(pvr_dma_callback) {
        /* This song and dance is necessary because the handler
           could chain to itself. */
        pvr_dma_callback_t cb = pvr_dma_callback;
        void *d = pvr_dma_cbdata;

        pvr_dma_callback = NULL;
        pvr_dma_cbdata = 0;

        cb(d);
    }

    /* Signal the calling thread to continue, if any. */
    if(pvr_dma_blocking) {
        sem_signal(&pvr_dma_done);
        thd_schedule(1, 0);
        pvr_dma_blocking = 0;
    }
}

int pvr_dma2_transfer(const void *sh4, pvr_ptr_t pvr, size_t length, uint32_t block, pvr_dma_callback_t callback, void *cbdata, uint32_t dir) {
    pvr_dma_blocking = block;
    pvr_dma_callback = callback;
    pvr_dma_cbdata = cbdata;

    /* Make sure we're not already DMA'ing */
    if(pvr_dma->start != 0) {
        dbglog(DBG_ERROR, "pvr_dma: Previous DMA has not finished\n");
        errno = EINPROGRESS;
        return -1;
    }

    pvr_dma->pvr_addr = (uintptr_t)pvr;
    pvr_dma->sh4_addr = (uintptr_t)sh4;
    pvr_dma->size = length;
    pvr_dma->dir = dir;
    pvr_dma->trigger_select = CPU_TRIGGER;

    /* Start the DMA transfer */
    pvr_dma->enable = 1;         
    pvr_dma->start = 1;          

    /* Wait for us to be signaled */
    if(block)
        sem_wait(&pvr_dma_done);

    return 0;
}

int pvr_dma_load_txr(pvr_ptr_t dest, const void *src, size_t length, int block,
                    pvr_dma_callback_t callback, void *cbdata) {
    uintptr_t dest_addr;

    dest_addr = ((uintptr_t)dest & 0xffffff) | PVR_RAM_BASE_64_P0;

    return pvr_dma2_transfer(src, (pvr_ptr_t)dest_addr, length, block, 
                            callback, cbdata, 0);
}

int pvr_dma_load_pal(pvr_ptr_t dest, const void *src, size_t length, int block,
                    pvr_dma_callback_t callback, void *cbdata) {
    uintptr_t dest_addr;

    dest_addr = ((uintptr_t)dest & 0xffffff) | PVR_RAM_BASE_64_P0;

    return pvr_dma2_transfer(src, (pvr_ptr_t)dest, length, block, 
                            callback, cbdata, 0);
}

int pvr_dma2_ready(void) {
    return pvr_dma->start == 0;
}

void pvr_dma2_init(void) {
    /* Create an initially blocked semaphore */
    sem_init(&pvr_dma_done, 0);
    pvr_dma_blocking = 0;
    pvr_dma_callback = NULL;
    pvr_dma_cbdata = 0;

    /* Hook the necessary interrupts */
    asic_evt_set_handler(ASIC_EVT_PVR_DMA, pvr_dma_irq_hnd, NULL);
    asic_evt_enable(ASIC_EVT_PVR_DMA, ASIC_IRQ_DEFAULT);

    *pvr_dma_pro = PVR_DMA_UNLOCK_ALLMEM;
}

void pvr_dma2_shutdown(void) {
    /* Need to ensure that no DMA is in progress */
    pvr_dma->enable = 0;

    /* Clean up */
    asic_evt_disable(ASIC_EVT_PVR_DMA, ASIC_IRQ_DEFAULT);
    asic_evt_remove_handler(ASIC_EVT_PVR_DMA);
    sem_destroy(&pvr_dma_done);

    *pvr_dma_pro = PVR_DMA_LOCK_ALLMEM;
}

/* Copies n bytes from src to PVR dest, dest must be 32-byte aligned */
void *pvr2_sq_load(void *dest, const void *src, size_t n, int type) {
    void *dma_area_ptr;

    if(pvr_dma->start != 0) {
        dbglog(DBG_ERROR, "pvr_sq_load: PVR DMA has not finished\n");
        errno = EINPROGRESS;
        return NULL;
    }

    dma_area_ptr = (void *)(((uintptr_t)dest & 0xffffff) | PVR_RAM_BASE_64_P0);
    sq_cpy(dma_area_ptr, src, n);

    return dest;
}

/* Fills n bytes at PVR dest with 16-bit c, dest must be 32-byte aligned */
void *pvr2_sq_set16(void *dest, uint32_t c, size_t n, int type) {
    void *dma_area_ptr;

    if(pvr_dma->start != 0) {
        dbglog(DBG_ERROR, "pvr_sq_set16: PVR DMA has not finished\n");
        errno = EINPROGRESS;
        return NULL;
    }

    dma_area_ptr = (void *)(((uintptr_t)dest & 0xffffff) | PVR_RAM_BASE_64_P0);
    sq_set16(dma_area_ptr, c, n);

    return dest;
}

/* Fills n bytes at PVR dest with 32-bit c, dest must be 32-byte aligned */
void *pvr2_sq_set32(void *dest, uint32_t c, size_t n, int type) {
    void *dma_area_ptr;

    if(pvr_dma->start != 0) {
        dbglog(DBG_ERROR, "pvr_sq_set32: PVR DMA has not finished\n");
        errno = EINPROGRESS;
        return NULL;
    }

    dma_area_ptr = (void *)(((uintptr_t)dest & 0xffffff) | PVR_RAM_BASE_64_P0);
    sq_set32(dma_area_ptr, c, n);

    return dest;
}