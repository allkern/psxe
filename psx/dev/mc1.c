#include <stdint.h>
#include <stdlib.h>

#include "mc1.h"
#include "../log.h"

psx_mc1_t* psx_mc1_create() {
    return (psx_mc1_t*)malloc(sizeof(psx_mc1_t));
}

/*
  1F801000h 4    Expansion 1 Base Address (usually 1F000000h)
  1F801004h 4    Expansion 2 Base Address (usually 1F802000h)
  1F801008h 4    Expansion 1 Delay/Size (usually 0013243Fh; 512Kbytes 8bit-bus)
  1F80100Ch 4    Expansion 3 Delay/Size (usually 00003022h; 1 byte)
  1F801010h 4    BIOS ROM    Delay/Size (usually 0013243Fh; 512Kbytes 8bit-bus)
  1F801014h 4    SPU_DELAY   Delay/Size (usually 200931E1h)
  1F801018h 4    CDROM_DELAY Delay/Size (usually 00020843h or 00020943h)
  1F80101Ch 4    Expansion 2 Delay/Size (usually 00070777h; 128-bytes 8bit-bus)
  1F801020h 4    COM_DELAY / COMMON_DELAY (00031125h or 0000132Ch or 00001325h)
*/
void psx_mc1_init(psx_mc1_t* mc1) {
    mc1->io_base = PSX_MC1_BEGIN;
    mc1->io_size = PSX_MC1_SIZE;

    mc1->exp1_base   = 0x1f000000;
    mc1->exp2_base   = 0x1f802000;
    mc1->exp1_delay  = 0x0013243f;
    mc1->exp3_delay  = 0x00003022;
    mc1->bios_delay  = 0x0013243f;
    mc1->spu_delay   = 0x200931e1;
    mc1->cdrom_delay = 0x00020843;
    mc1->exp2_delay  = 0x00070777;
    mc1->com_delay   = 0x00031125;
}

uint32_t psx_mc1_read32(psx_mc1_t* mc1, uint32_t offset) {
    switch (offset) {
        case 0x00: return mc1->exp1_base;
        case 0x04: return mc1->exp2_base;
        case 0x08: return mc1->exp1_delay;
        case 0x0c: return mc1->exp3_delay;
        case 0x10: return mc1->bios_delay;
        case 0x14: return mc1->spu_delay;
        case 0x18: return mc1->cdrom_delay;
        case 0x1c: return mc1->exp2_delay;
        case 0x20: return mc1->com_delay;
    }

    log_warn("Unhandled 32-bit MC1 read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_mc1_read16(psx_mc1_t* mc1, uint32_t offset) {
    log_warn("Unhandled 16-bit MC1 read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_mc1_read8(psx_mc1_t* mc1, uint32_t offset) {
    log_warn("Unhandled 8-bit MC1 read at offset %08x", offset);

    return 0x0;
}

void psx_mc1_write32(psx_mc1_t* mc1, uint32_t offset, uint32_t value) {
    switch (offset) {
        case 0x00: mc1->exp1_base = value; break;
        case 0x04: mc1->exp2_base = value; break;
        case 0x08: mc1->exp1_delay = value; break;
        case 0x0c: mc1->exp3_delay = value; break;
        case 0x10: mc1->bios_delay = value; break;
        case 0x14: mc1->spu_delay = value; break;
        case 0x18: mc1->cdrom_delay = value; break;
        case 0x1c: mc1->exp2_delay = value; break;
        case 0x20: mc1->com_delay = value; break;

        default: {
            log_warn("Unhandled 32-bit MC1 write at offset %08x (%08x)", offset, value);
        } break;
    }
}

void psx_mc1_write16(psx_mc1_t* mc1, uint32_t offset, uint16_t value) {
    log_warn("Unhandled 16-bit MC1 write at offset %08x (%04x)", offset, value);
}

void psx_mc1_write8(psx_mc1_t* mc1, uint32_t offset, uint8_t value) {
    log_warn("Unhandled 8-bit MC1 write at offset %08x (%02x)", offset, value);
}

void psx_mc1_destroy(psx_mc1_t* mc1) {
    free(mc1);
}