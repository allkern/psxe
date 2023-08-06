#include "mdec.h"
#include "../log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void mdec_nop(psx_mdec_t* mdec) { /* Do nothing */ }

void mdec_decode_macroblock(psx_mdec_t* mdec) {
    // To-do
}

void mdec_set_iqtab(psx_mdec_t* mdec) {
    mdec->state = mdec->cmd & 1 ? MDEC_RECV_QUANT_COLOR : MDEC_RECV_QUANT;
    mdec->data_remaining = mdec->cmd & 1 ? 32 : 16;
    mdec->index = 0;
}

void mdec_set_scale(psx_mdec_t* mdec) {
    mdec->state = MDEC_RECV_SCALE;
    mdec->data_remaining = 32;
    mdec->index = 0;
}

mdec_fn_t g_mdec_cmd_table[] = {
    mdec_nop,
    mdec_decode_macroblock,
    mdec_set_iqtab,
    mdec_set_scale,
    mdec_nop,
    mdec_nop,
    mdec_nop,
    mdec_nop
};

void mdec_recv_cmd(psx_mdec_t* mdec) {
    log_fatal("MDEC command %u (%08x)", mdec->cmd >> 29, mdec->cmd);

    g_mdec_cmd_table[mdec->cmd >> 29](mdec);
}
void mdec_recv_block(psx_mdec_t* mdec) {}
void mdec_recv_quant(psx_mdec_t* mdec) {
    mdec->data_remaining--;

    if (!mdec->data_remaining)
        mdec->state = MDEC_RECV_CMD;
}
void mdec_recv_quant_color(psx_mdec_t* mdec) {
    mdec->data_remaining--;

    if (!mdec->data_remaining)
        mdec->state = MDEC_RECV_CMD;
}
void mdec_recv_scale(psx_mdec_t* mdec) {
    mdec->data_remaining--;

    if (!mdec->data_remaining)
        mdec->state = MDEC_RECV_CMD;
}

mdec_fn_t g_mdec_recv_table[] = {
    mdec_recv_cmd,
    mdec_recv_block,
    mdec_recv_quant,
    mdec_recv_quant_color,
    mdec_recv_scale
};

psx_mdec_t* psx_mdec_create() {
    return (psx_mdec_t*)malloc(sizeof(psx_mdec_t));
}

void psx_mdec_init(psx_mdec_t* mdec) {
    memset(mdec, 0, sizeof(psx_mdec_t));

    mdec->io_base = PSX_MDEC_BEGIN;
    mdec->io_size = PSX_MDEC_SIZE;

    mdec->state = MDEC_RECV_CMD;
}

uint32_t psx_mdec_read32(psx_mdec_t* mdec, uint32_t offset) {
    switch (offset) {
        case 0: {

        } break;
        case 4: return mdec->status;
    }
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
    switch (offset) {
        case 0: {
            mdec->cmd = value;

            g_mdec_recv_table[mdec->state](mdec);
        } break;

        case 4: {
            if (value & 0x80000000)
                mdec->status = 0x80040000;

            mdec->status &= 0xe7ffffff;
            mdec->status |= (value & 0x60000000) >> 2;
        } break;
    }

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