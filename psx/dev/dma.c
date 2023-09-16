#include "dma.h"
#include "../log.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

psx_dma_t* psx_dma_create() {
    return (psx_dma_t*)malloc(sizeof(psx_dma_t));
}

const uint32_t g_psx_dma_ctrl_hw_1_table[] = {
    0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
    0x00000002
};

const uint32_t g_psx_dma_ctrl_hw_0_table[] = {
    0x71770703, 0x71770703, 0x71770703,
    0x71770703, 0x71770703, 0x71770703,
    0x50000002
};

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

void psx_dma_init(psx_dma_t* dma, psx_bus_t* bus, psx_ic_t* ic) {
    memset(dma, 0, sizeof(psx_dma_t));

    dma->io_base = PSX_DMAR_BEGIN;
    dma->io_size = PSX_DMAR_SIZE;

    dma->bus = bus;
    dma->ic = ic;

    dma->dpcr = 0x07654321;
}

uint32_t psx_dma_read32(psx_dma_t* dma, uint32_t offset) {
    if (offset < 0x70) {
        int channel = (offset >> 4) & 0x7;
        int reg = (offset >> 2) & 0x3;
        uint32_t cr = CR(channel, reg);

        if (reg == 2) {
            cr |= g_psx_dma_ctrl_hw_1_table[channel];
            cr &= g_psx_dma_ctrl_hw_0_table[channel];
        }

        log_error("DMA channel %u register %u (%08x) read %08x", channel, reg, PSX_DMAR_BEGIN + offset, cr);
        
        return cr;
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

        log_error("DMA channel %u register %u write (%08x) %08x", channel, reg, PSX_DMAR_BEGIN + offset, value);

        if (reg == 2)
            g_psx_dma_do_table[channel](dma);
    } else {
        switch (offset) {
            case 0x70: log_error("DMA control write %08x", value); dma->dpcr = value; break;
            case 0x74: {
                // IRQ signal is read-only
                value &= ~DICR_IRQSI;

                // Reset flags
                dma->dicr &= ~(value & DICR_FLAGS);

                // Write other fields
                uint32_t flags = dma->dicr & DICR_FLAGS;

                dma->dicr &= ~DICR_FLAGS;
                dma->dicr |= value & (~DICR_FLAGS);
                dma->dicr |= flags;
            } break;

            default: {
                log_error("Unhandled 32-bit DMA write at offset %08x (%08x)", offset, value);
            } break;
        }
    }
}

void psx_dma_write16(psx_dma_t* dma, uint32_t offset, uint16_t value) {
    switch (offset) {
        default: {
            log_fatal("Unhandled 16-bit DMA write at offset %08x (%04x)", offset, value);

            //exit(1);
        } break;
    }
}

void psx_dma_write8(psx_dma_t* dma, uint32_t offset, uint8_t value) {
    switch (offset) {
        default: {
            log_fatal("Unhandled 8-bit DMA write at offset %08x (%02x)", offset, value);

            //exit(1);
        } break;
    }
}

const char* g_psx_dma_sync_type_name_table[] = {
    "burst",
    "request",
    "linked",
    "reserved"
};

void psx_dma_do_mdec_in(psx_dma_t* dma) {
    if (!CHCR_BUSY(mdec_in))
        return;

    for (int i = 0; i < BCR_SIZE(mdec_in); i++) {
        uint32_t data = psx_bus_read32(dma->bus, dma->gpu.madr);

        psx_bus_write32(dma->bus, 0x1f801820, data);

        dma->gpu.madr += CHCR_STEP(mdec_in) ? -4 : 4;
    }

    dma->mdec_in.chcr &= ~(CHCR_BUSY_MASK | CHCR_TRIG_MASK);
    dma->mdec_in.bcr = 0;
}

void psx_dma_do_mdec_out(psx_dma_t* dma) {
    if (!CHCR_BUSY(mdec_out))
        return;

    log_fatal("MDEC_OUT DMA channel unimplemented");
}

void psx_dma_do_gpu_linked(psx_dma_t* dma) {
    uint32_t hdr = psx_bus_read32(dma->bus, dma->gpu.madr);
    uint32_t size = hdr >> 24;
    uint32_t addr = dma->gpu.madr;

    while (true) {
        while (size--) {
            addr = (addr + (CHCR_STEP(gpu) ? -4 : 4)) & 0x1ffffc;

            // Get command from linked list
            uint32_t cmd = psx_bus_read32(dma->bus, addr);

            // Write to GP0
            psx_bus_write32(dma->bus, 0x1f801810, cmd);

            dma->gpu_irq_delay++;
        }

        addr = hdr & 0xffffff;

        if (addr == 0xffffff) break;

        hdr = psx_bus_read32(dma->bus, addr);
        size = hdr >> 24;
    }
}

void psx_dma_do_gpu_request(psx_dma_t* dma) {
    if (!CHCR_BUSY(gpu))
        return;

    uint32_t size = BCR_SIZE(gpu) * BCR_BCNT(gpu);

    if (CHCR_TDIR(gpu)) {
        for (int i = 0; i < size; i++) {
            uint32_t data = psx_bus_read32(dma->bus, dma->gpu.madr);

            psx_bus_write32(dma->bus, 0x1f801810, data);

            dma->gpu.madr += CHCR_STEP(gpu) ? -4 : 4;
        }
    } else {
        for (int i = 0; i < size; i++) {
            uint32_t data = psx_bus_read32(dma->bus, 0x1f801810);

            psx_bus_write32(dma->bus, dma->gpu.madr, data);

            dma->gpu.madr += CHCR_STEP(gpu) ? -4 : 4;
        }
    }

    dma->gpu_irq_delay = size;
}

void psx_dma_do_gpu_burst(psx_dma_t* dma) {
    log_fatal("GPU DMA burst sync mode unimplemented");

    exit(1);
}

psx_dma_do_fn_t g_psx_dma_gpu_table[] = {
    psx_dma_do_gpu_burst,
    psx_dma_do_gpu_request,
    psx_dma_do_gpu_linked
};

#define TEST_SET_IRQ_FLAG()

void psx_dma_do_gpu(psx_dma_t* dma) {
    if (!CHCR_BUSY(gpu))
        return;

    // log_error("GPU DMA transfer: madr=%08x, dir=%s, sync=%s (%u), step=%s, size=%x",
    //     dma->gpu.madr,
    //     CHCR_TDIR(gpu) ? "to device" : "to RAM",
    //     g_psx_dma_sync_type_name_table[CHCR_SYNC(gpu)], CHCR_SYNC(gpu),
    //     CHCR_STEP(gpu) ? "decrementing" : "incrementing",
    //     BCR_SIZE(gpu)
    // );

    g_psx_dma_gpu_table[CHCR_SYNC(gpu)](dma);

    // Clear BCR and CHCR trigger and busy bits
    dma->gpu.chcr &= ~(CHCR_BUSY_MASK | CHCR_TRIG_MASK);
    dma->gpu.bcr = 0;
}

void psx_dma_do_cdrom(psx_dma_t* dma) {
    if (!CHCR_BUSY(cdrom))
        return;
    
    // log_fatal("CDROM DMA transfer: madr=%08x, dir=%s, sync=%s (%u), step=%s, size=%x",
    //     dma->cdrom.madr,
    //     CHCR_TDIR(cdrom) ? "to device" : "to RAM",
    //     g_psx_dma_sync_type_name_table[CHCR_SYNC(cdrom)], CHCR_SYNC(cdrom),
    //     CHCR_STEP(cdrom) ? "decrementing" : "incrementing",
    //     BCR_SIZE(cdrom)
    // );

    // log_fatal("DICR: force=%u, en=%02x, irqen=%u, flags=%02x",
    //     (dma->dicr >> 15) & 1,
    //     (dma->dicr >> 16) & 0x7f,
    //     (dma->dicr >> 23) & 1,
    //     (dma->dicr >> 24) & 0x7f
    // );

    uint32_t size = BCR_SIZE(cdrom);

    if (!size) {
        log_fatal("0 sized CDROM DMA");

        exit(1);
    }

    dma->cdrom_irq_delay = size * 24;

    if (!CHCR_TDIR(cdrom)) {
        for (int i = 0; i < size; i++) {
            uint32_t data = 0;
            
            data |= psx_bus_read8(dma->bus, 0x1f801802) << 0;
            data |= psx_bus_read8(dma->bus, 0x1f801802) << 8;
            data |= psx_bus_read8(dma->bus, 0x1f801802) << 16;
            data |= psx_bus_read8(dma->bus, 0x1f801802) << 24;

            psx_bus_write32(dma->bus, dma->cdrom.madr, data);

            dma->cdrom.madr += CHCR_STEP(cdrom) ? -4 : 4;
        }
    } else {
        log_fatal("Invalid CDROM DMA transfer direction");
    }
    
    // Clear BCR and CHCR trigger and busy bits
    dma->cdrom.chcr = 0;
    //dma->otc.chcr &= ~(CHCR_BUSY_MASK | CHCR_TRIG_MASK);
    dma->cdrom.bcr = 0;
}

void psx_dma_do_spu(psx_dma_t* dma) {
    if (!CHCR_BUSY(spu))
        return;
    
    // log_fatal("SPU DMA transfer: madr=%08x, dir=%s, sync=%s (%u), step=%s, size=%x",
    //     dma->spu.madr,
    //     CHCR_TDIR(spu) ? "to device" : "to RAM",
    //     g_psx_dma_sync_type_name_table[CHCR_SYNC(spu)], CHCR_SYNC(spu),
    //     CHCR_STEP(spu) ? "decrementing" : "incrementing",
    //     BCR_SIZE(spu)
    // );

    // log_fatal("DICR: force=%u, en=%02x, irqen=%u, flags=%02x",
    //     (dma->dicr >> 15) & 1,
    //     (dma->dicr >> 16) & 0x7f,
    //     (dma->dicr >> 23) & 1,
    //     (dma->dicr >> 24) & 0x7f
    // );

    uint32_t size = BCR_SIZE(spu) * BCR_BCNT(spu);

    if (!size) {
        log_fatal("0 sized SPU DMA");

        // exit(1);
    }

    dma->spu_irq_delay = size * 4;
    
    // Clear BCR and CHCR trigger and busy bits
    dma->spu.chcr = 0;
    dma->spu.bcr = 0;
}

void psx_dma_do_pio(psx_dma_t* dma) {
    log_fatal("PIO DMA channel unimplemented"); exit(1);
}

void psx_dma_do_otc(psx_dma_t* dma) {
    if ((!(dma->dpcr & DPCR_DMA6EN)) || (!CHCR_TRIG(otc)) || (!CHCR_BUSY(otc)))
        return;

    // log_fatal("OTC DMA transfer: madr=%08x, dir=%s, sync=%s, step=%s, size=%x",
    //     dma->otc.madr,
    //     CHCR_TDIR(otc) ? "to device" : "to RAM",
    //     CHCR_SYNC(otc) ? "other" : "burst",
    //     CHCR_STEP(otc) ? "decrementing" : "incrementing",
    //     BCR_SIZE(otc)
    // );

    uint32_t size = BCR_SIZE(otc);

    if (!size) size = 0x10000;

    for (int i = size; i > 0; i--) {
        uint32_t addr = (i != 1) ? (dma->otc.madr - 4) : 0xffffff;

        psx_bus_write32(dma->bus, dma->otc.madr, addr & 0xffffff);

        dma->otc.madr -= 4;
    }

    dma->otc_irq_delay = BCR_SIZE(otc);

    // Clear BCR and CHCR trigger and busy bits
    dma->otc.chcr = 0;
    //dma->otc.chcr &= ~(CHCR_BUSY_MASK | CHCR_TRIG_MASK);
    dma->otc.bcr = 0;
}

void psx_dma_update(psx_dma_t* dma, int cyc) {
    if (dma->cdrom_irq_delay) {
        if (dma->dicr & DICR_DMA3EN)
            dma->dicr |= DICR_DMA3FL;

        dma->cdrom_irq_delay = 0;
    }

    if (dma->spu_irq_delay) {
        if (dma->dicr & DICR_DMA4EN)
            dma->dicr |= DICR_DMA4FL;
        
        dma->spu_irq_delay = 0;
    }

    if (dma->gpu_irq_delay) {
        if (dma->dicr & DICR_DMA2EN)
            dma->dicr |= DICR_DMA2FL;

        dma->gpu_irq_delay = 0;
    }

    if (dma->otc_irq_delay) {
        if (dma->dicr & DICR_DMA6EN)
            dma->dicr |= DICR_DMA6FL;

        dma->otc_irq_delay = 0;
    }

    int prev_irq_signal = (dma->dicr & DICR_IRQSI) != 0;
    int irq_on_flags = (dma->dicr & DICR_IRQEN) != 0;
    int force_irq = (dma->dicr & DICR_FORCE) != 0;
    int irq = (dma->dicr & DICR_FLAGS) != 0;

    int irq_signal = force_irq || ((irq & irq_on_flags) != 0);

    if (irq_signal && !prev_irq_signal)
        psx_ic_irq(dma->ic, IC_DMA);
    
    dma->dicr &= ~DICR_IRQSI;
    dma->dicr |= irq_signal << 31;
}

void psx_dma_destroy(psx_dma_t* dma) {
    free(dma);
}

#undef CR