#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mc2.h"
#include "../log.h"

psx_mc2_t* psx_mc2_create(void) {
    return (psx_mc2_t*)malloc(sizeof(psx_mc2_t));
}

/*
  1F801060h 4/2  RAM_SIZE (usually 00000B88h; 2MB RAM mirrored in first 8MB)
*/
void psx_mc2_init(psx_mc2_t* mc2) {
    memset(mc2, 0, sizeof(psx_mc2_t));

    mc2->io_base = PSX_MC2_BEGIN;
    mc2->io_size = PSX_MC2_SIZE;

    mc2->ram_size = 0x00000b88;
}

uint32_t psx_mc2_read32(psx_mc2_t* mc2, uint32_t offset) {
    switch (offset) {
        case 0x00: return mc2->ram_size;
    }

    log_warn("Unhandled 32-bit MC2 read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_mc2_read16(psx_mc2_t* mc2, uint32_t offset) {
    log_warn("Unhandled 16-bit MC2 read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_mc2_read8(psx_mc2_t* mc2, uint32_t offset) {
    log_warn("Unhandled 8-bit MC2 read at offset %08x", offset);

    return 0x0;
}

void psx_mc2_write32(psx_mc2_t* mc2, uint32_t offset, uint32_t value) {
    switch (offset) {
        case 0x00: printf("ram_size write %08x\n", value); mc2->ram_size = value; break;

        default: {
            log_warn("Unhandled 32-bit MC2 write at offset %08x (%08x)", offset, value);
        } break;
    }
}

void psx_mc2_write16(psx_mc2_t* mc2, uint32_t offset, uint16_t value) {
    log_warn("Unhandled 16-bit MC2 write at offset %08x (%04x)", offset, value);
}

void psx_mc2_write8(psx_mc2_t* mc2, uint32_t offset, uint8_t value) {
    log_warn("Unhandled 8-bit MC2 write at offset %08x (%02x)", offset, value);
}

void psx_mc2_destroy(psx_mc2_t* mc2) {
    free(mc2);
}