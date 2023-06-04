#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "spu.h"
#include "../log.h"

psx_spu_t* psx_spu_create() {
    return (psx_spu_t*)malloc(sizeof(psx_spu_t));
}

void psx_spu_init(psx_spu_t* spu) {
    memset(spu, 0, sizeof(psx_spu_t));

    spu->io_base = PSX_SPU_BEGIN;
    spu->io_size = PSX_SPU_SIZE;
}

uint32_t psx_spu_read32(psx_spu_t* spu, uint32_t offset) {
    log_warn("Unhandled 32-bit SPU read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_spu_read16(psx_spu_t* spu, uint32_t offset) {
    log_warn("Unhandled 16-bit SPU read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_spu_read8(psx_spu_t* spu, uint32_t offset) {
    log_warn("Unhandled 8-bit SPU read at offset %08x", offset);

    return 0x0;
}

void psx_spu_write32(psx_spu_t* spu, uint32_t offset, uint32_t value) {
    log_warn("Unhandled 32-bit SPU write at offset %08x (%08x)", offset, value);
}

void psx_spu_write16(psx_spu_t* spu, uint32_t offset, uint16_t value) {
    log_warn("Unhandled 16-bit SPU write at offset %08x (%04x)", offset, value);
}

void psx_spu_write8(psx_spu_t* spu, uint32_t offset, uint8_t value) {
    log_warn("Unhandled 8-bit SPU write at offset %08x (%02x)", offset, value);
}

void psx_spu_destroy(psx_spu_t* spu) {
    free(spu);
}