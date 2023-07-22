#ifndef DMA_H
#define DMA_H

#include <stdint.h>

#define PSX_DMAR_BEGIN 0x1f801080
#define PSX_DMAR_SIZE  0x80
#define PSX_DMAR_END   0x1f8010ff

#include "../bus.h"
#include "ic.h"

typedef struct {
    uint32_t madr;
    uint32_t bcr;
    uint32_t chcr;
} dma_channel_t;

typedef struct {
    uint32_t io_base, io_size;

    psx_bus_t* bus;
    psx_ic_t* ic;

    dma_channel_t mdec_in;
    dma_channel_t mdec_out;
    dma_channel_t gpu;
    dma_channel_t cdrom;
    dma_channel_t spu;
    dma_channel_t pio;
    dma_channel_t otc;

    int cdrom_irq_delay;

    uint32_t dpcr;
    uint32_t dicr;
} psx_dma_t;

psx_dma_t* psx_dma_create();
void psx_dma_init(psx_dma_t*, psx_bus_t*, psx_ic_t*);
void psx_dma_do_mdec_in(psx_dma_t*);
void psx_dma_do_mdec_out(psx_dma_t*);
void psx_dma_do_gpu(psx_dma_t*);
void psx_dma_do_cdrom(psx_dma_t*);
void psx_dma_do_spu(psx_dma_t*);
void psx_dma_do_pio(psx_dma_t*);
void psx_dma_do_otc(psx_dma_t*);
void psx_dma_perform(psx_dma_t*, int);
uint32_t psx_dma_read32(psx_dma_t*, uint32_t);
uint16_t psx_dma_read16(psx_dma_t*, uint32_t);
uint8_t psx_dma_read8(psx_dma_t*, uint32_t);
void psx_dma_write32(psx_dma_t*, uint32_t, uint32_t);
void psx_dma_write16(psx_dma_t*, uint32_t, uint16_t);
void psx_dma_write8(psx_dma_t*, uint32_t, uint8_t);
void psx_dma_destroy(psx_dma_t*);
void psx_dma_update(psx_dma_t*, int);

typedef void (*psx_dma_do_fn_t)(psx_dma_t*);

/*
  0       Transfer Direction    (0=To Main RAM, 1=From Main RAM)
  1       Memory Address Step   (0=Forward;+4, 1=Backward;-4)
  2-7     Not used              (always zero)
  8       Chopping Enable       (0=Normal, 1=Chopping; run CPU during DMA gaps)
  9-10    SyncMode, Transfer Synchronisation/Mode (0-3):
            0  Start immediately and transfer all at once (used for CDROM, OTC)
            1  Sync blocks to DMA requests   (used for MDEC, SPU, and GPU-data)
            2  Linked-List mode              (used for GPU-command-lists)
            3  Reserved                      (not used)
  11-15   Not used              (always zero)
  16-18   Chopping DMA Window Size (1 SHL N words)
  19      Not used              (always zero)
  20-22   Chopping CPU Window Size (1 SHL N clks)
  23      Not used              (always zero)
  24      Start/Busy            (0=Stopped/Completed, 1=Start/Enable/Busy)
  25-27   Not used              (always zero)
  28      Start/Trigger         (0=Normal, 1=Manual Start; use for SyncMode=0)
  29      Unknown (R/W) Pause?  (0=No, 1=Pause?)     (For SyncMode=0 only?)
  30      Unknown (R/W)
  31      Not used              (always zero)
*/

#define CHCR_TDIR_MASK 0x00000001
#define CHCR_STEP_MASK 0x00000002
#define CHCR_CPEN_MASK 0x00000100
#define CHCR_SYNC_MASK 0x00000600
#define CHCR_CDWS_MASK 0x00070000
#define CHCR_CCWS_MASK 0x00380000
#define CHCR_BUSY_MASK 0x01000000
#define CHCR_TRIG_MASK 0x10000000

#define SYNC_SHIF 9
#define CDWS_SHIF 16
#define CCWS_SHIF 19

#define CHCR_TDIR(c) (dma->c.chcr & CHCR_TDIR_MASK)
#define CHCR_STEP(c) (dma->c.chcr & CHCR_STEP_MASK)
#define CHCR_CPEN(c) (dma->c.chcr & CHCR_CPEN_MASK)
#define CHCR_SYNC(c) ((dma->c.chcr & CHCR_SYNC_MASK) >> SYNC_SHIF)
#define CHCR_CDWS(c) ((dma->c.chcr & CHCR_CDWS_MASK) >> CDWS_SHIF)
#define CHCR_CCWS(c) ((dma->c.chcr & CHCR_CCWS_MASK) >> CCWS_SHIF)
#define CHCR_BUSY(c) (dma->c.chcr & CHCR_BUSY_MASK)
#define CHCR_TRIG(c) (dma->c.chcr & CHCR_TRIG_MASK)

#define BCR_SIZE(c) (dma->c.bcr & 0xffff)
#define BCR_BCNT(c) ((dma->c.bcr >> 16) & 0xffff)

#define DICR_FIRQ 0x00008000
#define DICR_IRQE 0x00000000

#endif