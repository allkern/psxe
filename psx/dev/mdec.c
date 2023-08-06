#include "mdec.h"
#include "../log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

psx_mdec_t* psx_mdec_create() {
    return (psx_mdec_t*)malloc(sizeof(psx_mdec_t));
}

void psx_mdec_init(psx_mdec_t* mdec) {
    memset(mdec, 0, sizeof(psx_mdec_t));

    mdec->io_base = PSX_MDEC_BEGIN;
    mdec->io_size = PSX_MDEC_SIZE;
}

uint32_t psx_mdec_read32(psx_mdec_t* mdec, uint32_t offset) {
    log_fatal("32-bit MDEC read offset=%u", offset);
}

uint16_t psx_mdec_read16(psx_mdec_t* mdec, uint32_t offset) {
    log_fatal("Unhandled 16-bit MDEC read offset=%u", offset);

    exit(1);
}

uint8_t psx_mdec_read8(psx_mdec_t* mdec, uint32_t offset) {
    log_fatal("Unhandled 8-bit MDEC read offset=%u", offset);

    exit(1);
}

void psx_mdec_write32(psx_mdec_t* mdec, uint32_t offset, uint32_t value) {
    log_fatal("32-bit MDEC write offset=%u, value=%08x", offset, value);
}

void psx_mdec_write16(psx_mdec_t* mdec, uint32_t offset, uint16_t value) {
    log_fatal("Unhandled 16-bit MDEC write offset=%u, value=%04x", offset, value);

    exit(1);
}

void psx_mdec_write8(psx_mdec_t* mdec, uint32_t offset, uint8_t value) {
    log_fatal("Unhandled 8-bit MDEC write offset=%u, value=%02x", offset, value);

    exit(1);
}

void psx_mdec_destroy(psx_mdec_t* mdec) {
    free(mdec);
}