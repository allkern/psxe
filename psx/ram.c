#include "ram.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

psx_ram_t* psx_ram_create() {
    return (psx_ram_t*)malloc(sizeof(psx_ram_t));
}

void psx_ram_init(psx_ram_t* ram) {
    ram->buf = (uint8_t*)malloc(PSX_RAM_SIZE);

    memset(ram->buf, 0xca, PSX_RAM_SIZE);
}

uint32_t psx_ram_read32(psx_ram_t* ram, uint32_t offset) {
    return *((uint32_t*)(ram->buf + offset));
}

uint16_t psx_ram_read16(psx_ram_t* ram, uint32_t offset) {
    return *((uint16_t*)(ram->buf + offset));
}

uint8_t psx_ram_read8(psx_ram_t* ram, uint32_t offset) {
    return ram->buf[offset];
}

void psx_ram_write32(psx_ram_t* ram, uint32_t offset, uint32_t value) {
    *((uint32_t*)(ram->buf + offset)) = value;
}

void psx_ram_write16(psx_ram_t* ram, uint32_t offset, uint16_t value) {
    *((uint16_t*)(ram->buf + offset)) = value;
}

void psx_ram_write8(psx_ram_t* ram, uint32_t offset, uint8_t value) {
    ram->buf[offset] = value;
}

void psx_ram_destroy(psx_ram_t* ram) {
    free(ram->buf);
    free(ram);
}