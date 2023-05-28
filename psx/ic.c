#include <stdint.h>
#include <stdlib.h>

#include "ic.h"
#include "log.h"

psx_ic_t* psx_ic_create() {
    return (psx_ic_t*)malloc(sizeof(psx_ic_t));
}

void psx_ic_init(psx_ic_t* ic) {
    ic->io_base = PSX_IC_BEGIN;
    ic->io_size = PSX_IC_SIZE;

    ic->i_stat = 0x00000000;
    ic->i_mask = 0x00000000;
}

uint32_t psx_ic_read32(psx_ic_t* ic, uint32_t offset) {
    switch (offset) {
        case 0x00: return ic->i_stat;
        case 0x04: return ic->i_mask;
    }

    log_warn("Unhandled 32-bit IC read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_ic_read16(psx_ic_t* ic, uint32_t offset) {
    switch (offset) {
        case 0x00: return ic->i_stat;
        case 0x04: return ic->i_mask;
    }

    log_warn("Unhandled 16-bit IC read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_ic_read8(psx_ic_t* ic, uint32_t offset) {
    log_warn("Unhandled 8-bit IC read at offset %08x", offset);

    return 0x0;
}

void psx_ic_write32(psx_ic_t* ic, uint32_t offset, uint32_t value) {
    switch (offset) {
        case 0x00: ic->i_stat = value; break;
        case 0x04: ic->i_mask = value; break;

        default: {
            log_warn("Unhandled 32-bit IC write at offset %08x (%08x)", offset, value);
        } break;
    }
}

void psx_ic_write16(psx_ic_t* ic, uint32_t offset, uint16_t value) {
    switch (offset) {
        case 0x00: ic->i_stat = value; break;
        case 0x04: ic->i_mask = value; break;

        default: {
            log_warn("Unhandled 16-bit IC write at offset %08x (%08x)", offset, value);
        } break;
    }
}

void psx_ic_write8(psx_ic_t* ic, uint32_t offset, uint8_t value) {
    log_warn("Unhandled 8-bit IC write at offset %08x (%02x)", offset, value);
}

void psx_ic_destroy(psx_ic_t* ic) {
    free(ic);
}