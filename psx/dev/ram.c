#include "ram.h"
#include "../log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

psx_ram_t* psx_ram_create(void) {
    return (psx_ram_t*)malloc(sizeof(psx_ram_t));
}

void psx_ram_init(psx_ram_t* ram, psx_mc2_t* mc2, int size) {
    memset(ram, 0, sizeof(psx_ram_t));

    ram->io_base = PSX_RAM_BEGIN;
    ram->io_size = PSX_RAM_SIZE;

    ram->mc2 = mc2;
    ram->buf = (uint8_t*)malloc(RAM_SIZE);

    if (size & 0x1ffff)
        size = RAM_SIZE_2MB;

    memset(ram->buf, RAM_INIT_FILL, size);
}

uint32_t psx_ram_read32(psx_ram_t* ram, uint32_t offset) {
    offset &= RAM_SIZE - 1;

    return *((uint32_t*)(ram->buf + offset));
}

uint16_t psx_ram_read16(psx_ram_t* ram, uint32_t offset) {
    offset &= RAM_SIZE - 1;

    return *((uint16_t*)(ram->buf + offset));
}

uint8_t psx_ram_read8(psx_ram_t* ram, uint32_t offset) {
    offset &= RAM_SIZE - 1;

    return ram->buf[offset];
}

void psx_ram_write32(psx_ram_t* ram, uint32_t offset, uint32_t value) {
    offset &= RAM_SIZE - 1;

    *((uint32_t*)(ram->buf + offset)) = value;
}

void psx_ram_write16(psx_ram_t* ram, uint32_t offset, uint16_t value) {
    offset &= RAM_SIZE - 1;

    *((uint16_t*)(ram->buf + offset)) = value;
}

void psx_ram_write8(psx_ram_t* ram, uint32_t offset, uint8_t value) {
    offset &= RAM_SIZE - 1;

    ram->buf[offset] = value;
}

void psx_ram_destroy(psx_ram_t* ram) {
    free(ram->buf);
    free(ram);
}