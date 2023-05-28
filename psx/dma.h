#ifndef DMA_H
#define DMA_H

#include <stdint.h>

#define PSX_DMAR_BEGIN 0x1f801080
#define PSX_DMAR_SIZE  0x80
#define PSX_DMAR_END   0x1f8010ff

typedef struct {
    uint32_t madr;
    uint32_t bcr;
    uint32_t chcr;
} dma_channel_t;

typedef struct {
    uint32_t io_base, io_size;

    dma_channel_t mdec_in;
    dma_channel_t mdec_out;
    dma_channel_t gpu;
    dma_channel_t cdrom;
    dma_channel_t spu;
    dma_channel_t pio;
    dma_channel_t otc;

    uint32_t dpcr;
    uint32_t dicr;
} psx_dma_t;

psx_dma_t* psx_dma_create();
void psx_dma_init(psx_dma_t*);
void psx_dma_perform(psx_dma_t*, int);
uint32_t psx_dma_read32(psx_dma_t*, uint32_t);
uint16_t psx_dma_read16(psx_dma_t*, uint32_t);
uint8_t psx_dma_read8(psx_dma_t*, uint32_t);
void psx_dma_write32(psx_dma_t*, uint32_t, uint32_t);
void psx_dma_write16(psx_dma_t*, uint32_t, uint16_t);
void psx_dma_write8(psx_dma_t*, uint32_t, uint8_t);
void psx_dma_destroy(psx_dma_t*);

#endif