#include "bios.h"
#include "../log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

psx_bios_t* psx_bios_create() {
    return (psx_bios_t*)malloc(sizeof(psx_bios_t));
}

void psx_bios_init(psx_bios_t* bios) {
    memset(bios, 0, sizeof(psx_bios_t));

    bios->io_base = PSX_BIOS_BEGIN;
    bios->io_size = PSX_BIOS_SIZE;
    bios->bus_delay = 20;

    bios->buf = (uint8_t*)malloc(PSX_BIOS_SIZE);
}

void psx_bios_load(psx_bios_t* bios, const char* path) {
    FILE* file = fopen(path, "rb");

    if (!file) {
        log_error("Couldn't open BIOS file \"%s\"", path);

        exit(1);
    }

    fread(bios->buf, 1, PSX_BIOS_SIZE, file);

    log_info("Loaded BIOS file \"%s\"", path);

    fclose(file);
}

uint32_t psx_bios_read32(psx_bios_t* bios, uint32_t offset) {
    return *((uint32_t*)(bios->buf + offset));
}

uint16_t psx_bios_read16(psx_bios_t* bios, uint32_t offset) {
    return *((uint16_t*)(bios->buf + offset));
}

uint8_t psx_bios_read8(psx_bios_t* bios, uint32_t offset) {
    return bios->buf[offset];
}

void psx_bios_write32(psx_bios_t* bios, uint32_t offset, uint32_t value) {
    log_warn("Unhandled 32-bit BIOS write at offset %08x (%08x)", offset, value);
}

void psx_bios_write16(psx_bios_t* bios, uint32_t offset, uint16_t value) {
    log_warn("Unhandled 16-bit BIOS write at offset %08x (%04x)", offset, value);
}

void psx_bios_write8(psx_bios_t* bios, uint32_t offset, uint8_t value) {
    log_warn("Unhandled 8-bit BIOS write at offset %08x (%02x)", offset, value);
}

void psx_bios_destroy(psx_bios_t* bios) {
    free(bios->buf);
    free(bios);
}