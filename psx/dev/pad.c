#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pad.h"
#include "../log.h"

psx_pad_t* psx_pad_create() {
    return (psx_pad_t*)malloc(sizeof(psx_pad_t));
}

void psx_pad_init(psx_pad_t* pad, psx_ic_t* ic) {
    memset(pad, 0, sizeof(psx_pad_t));

    pad->ic = ic;

    pad->io_base = PSX_PAD_BEGIN;
    pad->io_size = PSX_PAD_SIZE;

    pad->rx_buf[0] = 0xff;
    pad->rx_buf[1] = 0xff;
    pad->rx_buf[2] = 0x41;
    pad->rx_buf[3] = 0x5a;
    pad->rx_buf[4] = 0xff;
    pad->rx_buf[5] = 0xff;
}

uint32_t psx_pad_read32(psx_pad_t* pad, uint32_t offset) {
    // switch (offset) {
    //     case 0: return 0x00;
    //     case 4: return 0x01;
    //     case 8: return pad->mode;
    //     case 10: return pad->ctrl;
    //     case 14: return pad->baud;
    // }

    log_fatal("Unhandled 32-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_pad_read16(psx_pad_t* pad, uint32_t offset) {
    switch (offset) {
        // case 0: return 0x00;
        // case 4: return 0x03;
        // case 8: return pad->mode;
        case 10: return pad->ctrl;
        // case 14: return pad->baud;
    }

    log_fatal("Unhandled 16-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_pad_read8(psx_pad_t* pad, uint32_t offset) {
    switch (offset) {
        case 0: return pad->rx_buf[(pad->rx_index++) % 6];
        case 4: return 0x03;
        case 8: return pad->mode;
        case 10: return pad->ctrl;
        case 14: return pad->baud;
    }

    log_fatal("Unhandled 8-bit PAD read at offset %08x", offset);

    return 0x0;
}

void psx_pad_write32(psx_pad_t* pad, uint32_t offset, uint32_t value) {
    // switch (offset) {
    //     case 0: log_fatal("PAD write %08x", value); break;
    //     case 4: break;
    //     case 8: pad->mode = value & 0xffff; return;
    //     case 10: pad->ctrl = value & 0xffff; return;
    //     case 14: pad->baud = value & 0xffff; return;
    // }

    log_fatal("Unhandled 32-bit PAD write at offset %08x (%08x)", offset, value);
}

void psx_pad_write16(psx_pad_t* pad, uint32_t offset, uint16_t value) {
    switch (offset) {
        // case 0: break;
        // case 4: break;
        // case 8: pad->mode = value; return;
        case 10: pad->ctrl = value; return;
        // case 14: pad->baud = value; return;
    }

    log_fatal("Unhandled 16-bit PAD write at offset %08x (%04x)", offset, value);
}

void psx_pad_write8(psx_pad_t* pad, uint32_t offset, uint8_t value) {
    switch (offset) {
        case 0: return; break;
        case 4: break;
        case 8: pad->mode = value; return;
        case 10: pad->ctrl = value; return;
        case 14: pad->baud = value; return;
    }

    log_fatal("Unhandled 8-bit PAD write at offset %08x (%02x)", offset, value);
}

void psx_pad_button_press(psx_pad_t* pad, uint16_t data) {
    *((uint16_t*)(&pad->rx_buf[4])) &= ~data;
}

void psx_pad_button_release(psx_pad_t* pad, uint16_t data) {
    *((uint16_t*)(&pad->rx_buf[4])) |= data;
}

void psx_pad_destroy(psx_pad_t* pad) {
    free(pad);
}