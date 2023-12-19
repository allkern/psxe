#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ic.h"

#include "../log.h"

psx_ic_t* psx_ic_create() {
    return (psx_ic_t*)malloc(sizeof(psx_ic_t));
}

void psx_ic_init(psx_ic_t* ic, psx_cpu_t* cpu) {
    memset(ic, 0, sizeof(psx_ic_t));

    ic->io_base = PSX_IC_BEGIN;
    ic->io_size = PSX_IC_SIZE;

    ic->stat = 0x00000000;
    ic->mask = 0x00000000;

    ic->cpu = cpu;
}

uint32_t psx_ic_read32(psx_ic_t* ic, uint32_t offset) {
    switch (offset) {
        case 0x00: return ic->stat;
        case 0x04: return ic->mask;
    }

    log_fatal("Unhandled 32-bit IC read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_ic_read16(psx_ic_t* ic, uint32_t offset) {
    switch (offset) {
        case 0x00: return ic->stat;
        case 0x04: return ic->mask;
    }

    log_fatal("Unhandled 16-bit IC read at offset %08x", offset);

    exit(0);

    return 0x0;
}

uint8_t psx_ic_read8(psx_ic_t* ic, uint32_t offset) {
    log_fatal("Unhandled 8-bit IC read at offset %08x", offset);

    exit(0);

    return 0x0;
}

void psx_ic_write32(psx_ic_t* ic, uint32_t offset, uint32_t value) {
    switch (offset) {
        case 0x00: ic->stat &= value; break;
        case 0x04: ic->mask = value; break;

        default: {
            log_fatal("Unhandled 32-bit IC write at offset %08x (%08x)", offset, value);
        } break;
    }

    // Emulate acknowledge
    if (!(ic->stat & ic->mask)) {
        ic->cpu->cop0_r[COP0_CAUSE] &= ~SR_IM2;
    }
}

void psx_ic_write16(psx_ic_t* ic, uint32_t offset, uint16_t value) {
    switch (offset) {
        case 0x00: ic->stat &= (uint32_t)value; break;
        case 0x04: ic->mask &= 0xffff0000; ic->mask |= (uint32_t)value; break;

        default: {
            
        } break;
    }

    // Emulate acknowledge
    if (!(ic->stat & ic->mask)) {
        ic->cpu->cop0_r[COP0_CAUSE] &= ~SR_IM2;
    }
}

void psx_ic_write8(psx_ic_t* ic, uint32_t offset, uint8_t value) {
    printf("Unhandled 8-bit IC write at offset %08x (%02x)\n", offset, value);

    exit(1);
}

void psx_ic_irq(psx_ic_t* ic, int id) {
    ic->stat |= id;

    if (ic->mask & ic->stat)
        psx_cpu_set_irq_pending(ic->cpu);
}

void psx_ic_destroy(psx_ic_t* ic) {
    free(ic);
}