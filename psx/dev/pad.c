#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pad.h"
#include "../log.h"

uint32_t pad_read_rx(psx_pad_t* pad) {
    psx_input_t* joy = pad->joy_slot[(pad->ctrl >> 13) & 1];
    psx_mcd_t* mcd = pad->mcd_slot[(pad->ctrl >> 13) & 1];

    if ((!(joy || mcd)) || !pad->dest)
        return 0xffffffff;

    if (!(pad->ctrl & CTRL_JOUT) && !(pad->ctrl & CTRL_RXEN))
        return 0xffffffff;

    switch (pad->dest) {
        case DEST_JOY: {
            uint8_t data = joy->read_func(joy->udata);

            if (!joy->query_fifo_func(joy->udata))
                pad->dest = 0;

            return data;
        } break;

        case DEST_MCD: {
            uint8_t data = psx_mcd_read(mcd);

            if (!psx_mcd_query(mcd))
                pad->dest = 0;

            return data;
        } break;
    }

    return 0xffffffff;
}

void pad_write_tx(psx_pad_t* pad, uint16_t data) {
    psx_input_t* joy = pad->joy_slot[(pad->ctrl >> 13) & 1];
    psx_mcd_t* mcd = pad->mcd_slot[(pad->ctrl >> 13) & 1];

    if ((!(joy || mcd)) || !(pad->ctrl & CTRL_TXEN))
        return;

    if (pad->ctrl & CTRL_ACIE)
        pad->cycles_until_irq = 1500;

    if (!pad->dest) {
        if ((data == DEST_JOY) || (data == DEST_MCD))
            pad->dest = data;
    } else {
        switch (pad->dest) {
            case DEST_JOY: {
                joy->write_func(joy->udata, data);

                if (!joy->query_fifo_func(joy->udata))
                    pad->dest = 0;
            } break;

            case DEST_MCD: {
                psx_mcd_write(mcd, data);

                if (!psx_mcd_query(mcd))
                    pad->dest = 0;
            } break;
        }
    }
}

uint32_t pad_handle_stat_read(psx_pad_t* pad) {
    // log_set_quiet(0);
    // log_fatal("pad stat read");
    // log_set_quiet(1);
    return 0x07;
    psx_input_t* joy = pad->joy_slot[(pad->ctrl >> 13) & 1];

    if (!joy)
        return 0x5 | pad->stat;

    return 0x5 | (joy->query_fifo_func(joy->udata) << 1);
}

void pad_handle_ctrl_write(psx_pad_t* pad, uint32_t value) {
    pad->ctrl = value;

    if (!(pad->ctrl & CTRL_JOUT)) {
        pad->dest = 0;
        pad->ctrl &= ~CTRL_SLOT;

        psx_mcd_reset(pad->mcd_slot[(pad->ctrl >> 13) & 1]);
    }
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
        case 0: return pad_read_rx(pad);
        case 4: return pad_handle_stat_read(pad);
        case 8: return pad->mode;
        case 10: return pad->ctrl;
        case 14: return pad->baud;
    }

    log_fatal("Unhandled 32-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_pad_read16(psx_pad_t* pad, uint32_t offset) {
    switch (offset) {
        case 0: return pad_read_rx(pad) & 0xffff;
        case 4: return pad_handle_stat_read(pad) & 0xffff;
        case 8: return pad->mode;
        case 10: return pad->ctrl & 0xffff;
        case 14: return pad->baud;
    }

    log_fatal("Unhandled 16-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_pad_read8(psx_pad_t* pad, uint32_t offset) {
    switch (offset) {
        case 0: return pad_read_rx(pad) & 0xff;
        case 4: return pad_handle_stat_read(pad) & 0xff;
        case 8: return pad->mode & 0xff;
        case 10: return pad->ctrl & 0xff;
        case 14: return pad->baud & 0xff;
    }

    log_fatal("Unhandled 8-bit PAD read at offset %08x", offset);

    return 0x0;
}

void psx_pad_write32(psx_pad_t* pad, uint32_t offset, uint32_t value) {
    switch (offset) {
        case 0: pad_write_tx(pad, value); return;
        case 8: pad->mode = value & 0xffff; return;
        case 10: pad_handle_ctrl_write(pad, value); return;
        case 14: pad->baud = value & 0xffff; return;
    }

    log_fatal("Unhandled 32-bit PAD write at offset %08x (%08x)", offset, value);
}

void psx_pad_write16(psx_pad_t* pad, uint32_t offset, uint16_t value) {
    switch (offset) {
        case 0: pad_write_tx(pad, value); return;
        case 8: pad->mode = value; return;
        case 10: pad_handle_ctrl_write(pad, value); return;
        case 14: pad->baud = value; return;
    }

    log_fatal("Unhandled 16-bit PAD write at offset %08x (%04x)", offset, value);
}

void psx_pad_write8(psx_pad_t* pad, uint32_t offset, uint8_t value) {
    switch (offset) {
        case 0: pad_write_tx(pad, value); return;
        case 8: pad->mode = value; return;
        case 10: pad_handle_ctrl_write(pad, value); return;
        case 14: pad->baud = value; return;
    }

    log_fatal("Unhandled 8-bit PAD write at offset %08x (%02x)", offset, value);
}

void psx_pad_button_press(psx_pad_t* pad, int slot, uint16_t data) {
    psx_input_t* selected_slot = pad->joy_slot[slot];

    if (selected_slot)
        selected_slot->on_button_press_func(selected_slot->udata, data);
}

void psx_pad_button_release(psx_pad_t* pad, int slot, uint16_t data) {
    psx_input_t* selected_slot = pad->joy_slot[slot];

    if (selected_slot)
        selected_slot->on_button_release_func(selected_slot->udata, data);
}

void psx_pad_attach_joy(psx_pad_t* pad, int slot, psx_input_t* input) {
    if (pad->joy_slot[slot])
        psx_pad_detach_joy(pad, slot);

    pad->joy_slot[slot] = input;
}

void psx_pad_detach_joy(psx_pad_t* pad, int slot) {
    if (!pad->joy_slot[slot])
        return;

    psx_input_destroy(pad->joy_slot[slot]);

    pad->joy_slot[slot] = NULL;
}

void psx_pad_attach_mcd(psx_pad_t* pad, int slot, const char* path) {
    if (pad->mcd_slot[slot])
        psx_pad_detach_mcd(pad, slot);

    psx_mcd_t* mcd = psx_mcd_create();
    psx_mcd_init(mcd, path);

    pad->mcd_slot[slot] = mcd;
}

void psx_pad_detach_mcd(psx_pad_t* pad, int slot) {
    if (!pad->mcd_slot[slot])
        return;

    psx_mcd_destroy(pad->mcd_slot[slot]);

    pad->mcd_slot[slot] = NULL;
}

void psx_pad_update(psx_pad_t* pad, int cyc) {
    if (pad->cycles_until_irq) {
        pad->cycles_until_irq -= cyc;

        if (pad->cycles_until_irq <= 0) {
            psx_ic_irq(pad->ic, IC_JOY);

            pad->cycles_until_irq = 0;
        }
    }
}

void psx_pad_destroy(psx_pad_t* pad) {
    psx_pad_detach_joy(pad, 0);
    psx_pad_detach_joy(pad, 1);
    psx_pad_detach_mcd(pad, 0);
    psx_pad_detach_mcd(pad, 1);

    free(pad);
}