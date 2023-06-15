#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pad.h"
#include "../log.h"

psx_pad_t* psx_pad_create() {
    return (psx_pad_t*)malloc(sizeof(psx_pad_t));
}

void psx_pad_init(psx_pad_t* pad) {
    memset(pad, 0, sizeof(psx_pad_t));

    pad->io_base = PSX_PAD_BEGIN;
    pad->io_size = PSX_PAD_SIZE;
}

uint32_t psx_pad_read32(psx_pad_t* pad, uint32_t offset) {
    log_fatal("PAD read32 %u", offset);

    if (offset == 0x4) return 0x00000005;

    log_warn("Unhandled 32-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_pad_read16(psx_pad_t* pad, uint32_t offset) {
    log_fatal("PAD read16 %u", offset);
    if (offset == 0x4) return 0x00000005;

    log_warn("Unhandled 16-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_pad_read8(psx_pad_t* pad, uint32_t offset) {
    log_fatal("PAD read8 %u", offset);
    if (offset == 0x4) return 0x00000005;

    log_warn("Unhandled 8-bit PAD read at offset %08x", offset);

    return 0x0;
}

void psx_pad_write32(psx_pad_t* pad, uint32_t offset, uint32_t value) {
    log_warn("Unhandled 32-bit PAD write at offset %08x (%08x)", offset, value);
}

void psx_pad_write16(psx_pad_t* pad, uint32_t offset, uint16_t value) {
    log_warn("Unhandled 16-bit PAD write at offset %08x (%04x)", offset, value);
}

void psx_pad_write8(psx_pad_t* pad, uint32_t offset, uint8_t value) {
    log_warn("Unhandled 8-bit PAD write at offset %08x (%02x)", offset, value);
}

void psx_pad_destroy(psx_pad_t* pad) {
    free(pad);
}