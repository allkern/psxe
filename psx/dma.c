#include "dma.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>

psx_dma_t* psx_dma_create() {
    return (psx_dma_t*)malloc(sizeof(psx_dma_t));
}

#define CR(c, r) *((&dma->mdec_in.madr) + (c * 3) + r)

void psx_dma_init(psx_dma_t* dma) {
    dma->io_base = PSX_DMAR_BEGIN;
    dma->io_size = PSX_DMAR_SIZE;

    dma->dpcr = 0x07654321;
    dma->otc.chcr = 0x00000002;
}

uint32_t psx_dma_read32(psx_dma_t* dma, uint32_t offset) {
    if (offset < 0x70) {
        int channel = (offset >> 4) & 0x7;
        int reg = (offset >> 2) & 0x3;

        log_error("DMA channel %u register %u read %08x", channel, reg, (CR(channel, reg)));
        
        return CR(channel, reg);
    } else {
        switch (offset) {
            case 0x70: log_error("DMA control read %08x", dma->dpcr); return dma->dpcr;
            case 0x74: log_error("DMA irqc    read %08x", dma->dicr); return dma->dicr;

            default: {
                log_error("Unhandled 32-bit DMA read at offset %08x", offset);

                return 0x0;
            }
        }
    }
}

uint16_t psx_dma_read16(psx_dma_t* dma, uint32_t offset) {
    switch (offset) {
        case 0x70: return dma->dpcr;
        case 0x74: return dma->dicr;

        default: {
            log_error("Unhandled 16-bit DMA read at offset %08x", offset);

            return 0x0;
        }
    }
}

uint8_t psx_dma_read8(psx_dma_t* dma, uint32_t offset) {
    switch (offset) {
        case 0x70: return dma->dpcr;
        case 0x74: return dma->dicr;

        default: {
            log_error("Unhandled 8-bit DMA read at offset %08x", offset);

            return 0x0;
        }
    }
}

void psx_dma_write32(psx_dma_t* dma, uint32_t offset, uint32_t value) {
    if (offset < 0x70) {
        int channel = (offset >> 4) & 0x7;
        int reg = (offset >> 2) & 0x3;

        CR(channel, reg) = value;

        if (reg == 2) {
            psx_dma_perform(dma, channel);
        }

        log_error("DMA channel %u register %u write %08x", channel, reg, value);
    } else {
        switch (offset) {
            case 0x70: log_error("DMA control write %08x", value); dma->dpcr = value; break;
            case 0x74: log_error("DMA irqc    write %08x", value); dma->dicr = value; break;

            default: {
                log_error("Unhandled 32-bit DMA write at offset %08x (%08x)", offset, value);
            } break;
        }
    }
}

void psx_dma_write16(psx_dma_t* dma, uint32_t offset, uint16_t value) {
    switch (offset) {
        default: {
            log_error("Unhandled 16-bit DMA write at offset %08x (%04x)", offset, value);
        } break;
    }
}

void psx_dma_write8(psx_dma_t* dma, uint32_t offset, uint8_t value) {
    switch (offset) {
        default: {
            log_error("Unhandled 8-bit DMA write at offset %08x (%02x)", offset, value);
        } break;
    }
}

void psx_dma_perform(psx_dma_t* dma, int channel) {
    int mode = ((CR(channel, 2)) >> 28) & 3;

    if (mode) {
        log_error("Unimplemented DMA%u sync mode %u", channel, mode);

        return;
    }

    int trigger = ((CR(channel, 2)) >> 28) & 1;

    if (trigger) {
        // To-do
    }
}

void psx_dma_destroy(psx_dma_t* dma) {
    free(dma);
}

#undef CR