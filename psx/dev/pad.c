#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pad.h"
#include "../log.h"

uint32_t pad_read_rx(psx_pad_t* pad) {
    psx_input_t* current_slot = pad->slot[(pad->ctrl >> 13) & 1];

    if (!current_slot)
        return 0xffffffff;

    if (!(pad->ctrl & CTRL_JOUT)) {
        if (pad->ctrl & CTRL_RXEN) {
            uint32_t data = current_slot->read_func(current_slot);

            pad->ctrl &= ~CTRL_RXEN;

            return data;
        }

        return 0xffffffff;
    }

    return current_slot->read_func(current_slot->udata);
}

void pad_write_tx(psx_pad_t* pad, uint16_t data) {
    psx_input_t* current_slot = pad->slot[(pad->ctrl >> 13) & 1];

    if (!current_slot)
        return;

    if (!(pad->ctrl & CTRL_TXEN))
        return;
    
    pad->cycles_until_irq = 512;
    
    current_slot->write_func(current_slot->udata, data);
}

uint32_t pad_handle_stat_read(psx_pad_t* pad) {
    return 0x07;
    psx_input_t* current_slot = pad->slot[(pad->ctrl >> 13) & 1];

    if (!current_slot)
        return 0x5 | pad->stat;

    return 0x5 | (current_slot->query_fifo_func(current_slot->udata) << 1);
}

void pad_handle_ctrl_write(psx_pad_t* pad, uint32_t value) {
    pad->ctrl = value;
}

psx_pad_t* psx_pad_create() {
    return (psx_pad_t*)malloc(sizeof(psx_pad_t));
}

void psx_pad_init(psx_pad_t* pad, psx_ic_t* ic) {
    memset(pad, 0, sizeof(psx_pad_t));

    pad->ic = ic;

    pad->io_base = PSX_PAD_BEGIN;
    pad->io_size = PSX_PAD_SIZE;
}

uint32_t psx_pad_read32(psx_pad_t* pad, uint32_t offset) {
    switch (offset) {
        case 0: log_fatal("RX read 32"); return pad_read_rx(pad);
        case 4: log_fatal("ST read 32"); return pad_handle_stat_read(pad);
        case 8: log_fatal("MD read 32"); return pad->mode;
        case 10: log_fatal("CT read 32"); return pad->ctrl;
        case 14: log_fatal("BD read 32"); return pad->baud;
    }

    log_fatal("Unhandled 32-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_pad_read16(psx_pad_t* pad, uint32_t offset) {
    switch (offset) {
        case 0: log_fatal("RX read 16"); return pad_read_rx(pad) & 0xffff;
        case 4: log_fatal("ST read 16 %04x", pad_handle_stat_read(pad) & 0xffff); return pad_handle_stat_read(pad) & 0xffff;
        case 8: log_fatal("MD read 16"); return pad->mode;
        case 10: log_fatal("CT read 16 %04x", pad->ctrl & 0xffff); return pad->ctrl & 0xffff;
        case 14: log_fatal("BD read 16"); return pad->baud;
    }

    log_fatal("Unhandled 16-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_pad_read8(psx_pad_t* pad, uint32_t offset) {
    switch (offset) {
        case 0: log_fatal("RX read 8 %02x", pad_read_rx(pad) & 0xff); return pad_read_rx(pad) & 0xff;
        case 4: log_fatal("ST read 8"); return pad_handle_stat_read(pad) & 0xff;
        case 8: log_fatal("MD read 8"); return pad->mode & 0xff;
        case 10: log_fatal("CT read 8"); return pad->ctrl & 0xff;
        case 14: log_fatal("BD read 8"); return pad->baud & 0xff;
    }

    log_fatal("Unhandled 8-bit PAD read at offset %08x", offset);

    return 0x0;
}

void psx_pad_write32(psx_pad_t* pad, uint32_t offset, uint32_t value) {
    switch (offset) {
        case 0: log_fatal("TX write 32 %08x", value); pad_write_tx(pad, value); return;
        case 8: log_fatal("MD write 32 %08x", value); pad->mode = value & 0xffff; return;
        case 10: log_fatal("CT write 32 %08x", value); pad_handle_ctrl_write(pad, value); return;
        case 14: log_fatal("BD write 32 %08x", value); pad->baud = value & 0xffff; return;
    }

    log_fatal("Unhandled 32-bit PAD write at offset %08x (%08x)", offset, value);
}

void psx_pad_write16(psx_pad_t* pad, uint32_t offset, uint16_t value) {
    switch (offset) {
        case 0: log_fatal("TX write 16 %04x", value); pad_write_tx(pad, value); return;
        case 8: log_fatal("MD write 16 %04x", value); pad->mode = value; return;
        case 10: log_fatal("CT write 16 %04x", value); pad_handle_ctrl_write(pad, value); return;
        case 14: log_fatal("BD write 16 %04x", value); pad->baud = value; return;
    }

    log_fatal("Unhandled 16-bit PAD write at offset %08x (%04x)", offset, value);
}

void psx_pad_write8(psx_pad_t* pad, uint32_t offset, uint8_t value) {
    switch (offset) {
        case 0: log_fatal("TX write 8 %02x", value); pad_write_tx(pad, value); return;
        case 8: log_fatal("MD write 8 %02x", value); pad->mode = value; return;
        case 10: log_fatal("CT write 8 %02x", value); pad_handle_ctrl_write(pad, value); return;
        case 14: log_fatal("BD write 8 %02x", value); pad->baud = value; return;
    }

    log_fatal("Unhandled 8-bit PAD write at offset %08x (%02x)", offset, value);
}

void psx_pad_button_press(psx_pad_t* pad, int slot, uint16_t data) {
    psx_input_t* selected_slot = pad->slot[slot];

    if (selected_slot)
        selected_slot->on_button_press_func(selected_slot->udata, data);
}

void psx_pad_button_release(psx_pad_t* pad, int slot, uint16_t data) {
    psx_input_t* selected_slot = pad->slot[slot];

    if (selected_slot)
        selected_slot->on_button_release_func(selected_slot->udata, data);
}

void psx_pad_init_slot(psx_pad_t* pad, int slot, psx_input_t* input) {
    pad->slot[slot] = input;
}

void psx_pad_update(psx_pad_t* pad, int cyc) {
    if (pad->cycles_until_irq) {
        pad->cycles_until_irq -= cyc;

        if (pad->cycles_until_irq <= 0) {
            psx_ic_irq(pad->ic, IC_JOY);

            log_fatal("PAD IRQ");

            pad->cycles_until_irq = 0;
        }
    }
}

void psx_pad_destroy(psx_pad_t* pad) {
    free(pad);
}