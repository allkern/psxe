#include "ram.h"
#include "../log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

psx_ram_t* psx_ram_create() {
    return (psx_ram_t*)malloc(sizeof(psx_ram_t));
}

void psx_ram_init(psx_ram_t* ram, psx_mc2_t* mc2) {
    memset(ram, 0, sizeof(psx_ram_t));

    ram->io_base = PSX_RAM_BEGIN;
    ram->io_size = PSX_RAM_SIZE;

    ram->mc2 = mc2;
    ram->buf = (uint8_t*)malloc(PSX_RAM_SIZE);

    memset(ram->buf, 0xee, PSX_RAM_SIZE);
}

uint32_t psx_ram_read32(psx_ram_t* ram, uint32_t offset) {
    offset &= 0x1fffff;

    return *((uint32_t*)(ram->buf + offset));
}

uint16_t psx_ram_read16(psx_ram_t* ram, uint32_t offset) {
    offset &= 0x1fffff;

    return *((uint16_t*)(ram->buf + offset));
}

uint8_t psx_ram_read8(psx_ram_t* ram, uint32_t offset) {
    offset &= 0x1fffff;

    return ram->buf[offset];
}

void psx_ram_write32(psx_ram_t* ram, uint32_t offset, uint32_t value) {
    offset &= 0x1fffff;

    *((uint32_t*)(ram->buf + offset)) = value;
}

void psx_ram_write16(psx_ram_t* ram, uint32_t offset, uint16_t value) {
    offset &= 0x1fffff;

    *((uint16_t*)(ram->buf + offset)) = value;
}

void psx_ram_write8(psx_ram_t* ram, uint32_t offset, uint8_t value) {
    offset &= 0x1fffff;

    ram->buf[offset] = value;
}

void psx_ram_destroy(psx_ram_t* ram) {
    free(ram->buf);
    free(ram);
}