#include "dma.h"
#include "../log.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

psx_dma_t* psx_dma_create() {
    return (psx_dma_t*)malloc(sizeof(psx_dma_t));
}

const psx_dma_do_fn_t g_psx_dma_do_table[] = {
    psx_dma_do_mdec_in,
    psx_dma_do_mdec_out,
    psx_dma_do_gpu,
    psx_dma_do_cdrom,
    psx_dma_do_spu,
    psx_dma_do_pio,
    psx_dma_do_otc
};

#define CR(c, r) *((&dma->mdec_in.madr) + (c * 3) + r)

void psx_dma_init(psx_dma_t* dma, psx_bus_t* bus) {
    dma->io_base = PSX_DMAR_BEGIN;
    dma->io_size = PSX_DMAR_SIZE;

    dma->bus = bus;

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

        log_error("DMA channel %u register %u write %08x", channel, reg, value);

        if (reg == 2) {
            g_psx_dma_do_table[channel](dma);
        }
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

const char* g_psx_dma_sync_type_name_table[] = {
    "burst",
    "request",
    "linked",
    "reserved"
};

void psx_dma_do_mdec_in(psx_dma_t* dma) { log_error("MDEC_IN DMA channel unimplemented"); }
void psx_dma_do_mdec_out(psx_dma_t* dma) { log_error("MDEC_OUT DMA channel unimplemented"); }

void psx_dma_do_gpu_linked(psx_dma_t* dma) {
    uint32_t hdr = psx_bus_read32(dma->bus, dma->gpu.madr);
    uint32_t size = hdr >> 24;
    uint32_t addr = dma->gpu.madr;

    log_error("GPU packet hdr=%08x, size=%02x", hdr, size);

    while (true) {
        while (size--) {
            addr = (addr + 4) & 0x1ffffc;

            // Get command from linked list
            uint32_t cmd = psx_bus_read32(dma->bus, addr);

            // Write to GP0
            psx_bus_write32(dma->bus, 0x1f801810, cmd);
        }

        addr = hdr & 0xffffff;

        if (addr == 0xffffff) break;

        hdr = psx_bus_read32(dma->bus, addr);
        size = hdr >> 24;

        log_error("GPU packet hdr=%08x, size=%02x", hdr, size);
    }
}

void psx_dma_do_gpu_request(psx_dma_t* dma) {
    log_error("GPU DMA request sync mode unimplemented");

    exit(1);
}

void psx_dma_do_gpu_burst(psx_dma_t* dma) {
    log_error("GPU DMA burst sync mode unimplemented");

    exit(1);
}

psx_dma_do_fn_t g_psx_dma_gpu_table[] = {
    psx_dma_do_gpu_burst,
    psx_dma_do_gpu_request,
    psx_dma_do_gpu_linked
};

void psx_dma_do_gpu(psx_dma_t* dma) {
    if (!CHCR_BUSY(gpu))
        return;

    log_error("GPU DMA transfer: madr=%08x, dir=%s, sync=%s (%u), step=%s, size=%x",
        dma->gpu.madr,
        CHCR_TDIR(gpu) ? "to device" : "to RAM",
        g_psx_dma_sync_type_name_table[CHCR_SYNC(gpu)], CHCR_SYNC(gpu),
        CHCR_STEP(gpu) ? "decrementing" : "incrementing",
        BCR_SIZE(gpu)
    );

    g_psx_dma_gpu_table[CHCR_SYNC(gpu)](dma);

    // Clear BCR and CHCR trigger and busy bits
    dma->gpu.chcr &= ~(CHCR_BUSY_MASK | CHCR_TRIG_MASK);
    dma->gpu.bcr = 0;
}

void psx_dma_do_cdrom(psx_dma_t* dma) { log_error("CDROM DMA channel unimplemented"); exit(1); }
void psx_dma_do_spu(psx_dma_t* dma) { log_error("SPU DMA channel unimplemented"); exit(1); }
void psx_dma_do_pio(psx_dma_t* dma) { log_error("PIO DMA channel unimplemented"); exit(1); }
void psx_dma_do_otc(psx_dma_t* dma) {
    if (!CHCR_TRIG(otc))
        return;

    assert(!CHCR_TDIR(otc));
    assert(CHCR_SYNC(otc) == 0);
    assert(CHCR_STEP(otc));
    assert(BCR_SIZE(otc));

    log_error("OTC DMA transfer: madr=%08x, dir=%s, sync=%s, step=%s, size=%x",
        dma->otc.madr,
        CHCR_TDIR(otc) ? "to device" : "to RAM",
        CHCR_SYNC(otc) ? "other" : "burst",
        CHCR_STEP(otc) ? "decrementing" : "incrementing",
        BCR_SIZE(otc)
    );

    for (int i = BCR_SIZE(otc); i > 0; i--) {
        psx_bus_write32(dma->bus, dma->otc.madr, (i != 1) ? (dma->otc.madr - 4) : 0xffffff);

        dma->otc.madr -= 4;
    }

    // Clear BCR and CHCR trigger and busy bits
    dma->otc.chcr &= ~(CHCR_BUSY_MASK | CHCR_TRIG_MASK);
    dma->otc.bcr = 0;
}

void psx_dma_perform(psx_dma_t* dma, int channel) {
    switch (channel) {
        case 0: psx_dma_do_mdec_in(dma); break;
        case 1: psx_dma_do_mdec_out(dma); break;
        case 2: psx_dma_do_gpu(dma); break;
        case 3: psx_dma_do_cdrom(dma); break;
        case 4: psx_dma_do_spu(dma); break;
        case 5: psx_dma_do_pio(dma); break;
        case 6: psx_dma_do_otc(dma); break;
    }
}

void psx_dma_destroy(psx_dma_t* dma) {
    free(dma);
}

#undef CR