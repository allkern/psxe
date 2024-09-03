#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pad.h"
#include "../log.h"

#define JOY_IRQ_DELAY 512

uint32_t pad_read_rx(psx_pad_t* pad) {
    int slot = (pad->ctrl >> 13) & 1;

    psx_input_t* joy = pad->joy_slot[slot];
    psx_mcd_t* mcd = pad->mcd_slot[slot];

    if (!pad->dest[slot])
        return 0xffffffff;

    if (!(pad->ctrl & CTRL_JOUT) && !(pad->ctrl & CTRL_RXEN))
        return 0xffffffff;

    switch (pad->dest[slot]) {
        case DEST_JOY: {
            if (!joy) {
                pad->dest[slot] = 0;

                return 0xffffffff;
            }

            uint8_t data = joy->read_func(joy->udata);

            if (!joy->query_fifo_func(joy->udata))
                pad->dest[slot] = 0;

            return data;
        } break;

        case DEST_MCD: {
            if (!mcd) {
                pad->dest[slot] = 0;

                return 0xffffffff;
            }

            uint8_t data = psx_mcd_read(mcd);

            if (!psx_mcd_query(mcd))
                pad->dest[slot] = 0;

            return data;
        } break;
    }

    return 0xffffffff;
}

void pad_write_tx(psx_pad_t* pad, uint16_t data) {
    int slot = (pad->ctrl >> 13) & 1;

    psx_input_t* joy = pad->joy_slot[slot];
    psx_mcd_t* mcd = pad->mcd_slot[slot];

    if (!(pad->ctrl & CTRL_TXEN))
        return;

    if (!pad->dest[slot]) {
        if ((data == DEST_JOY) || (data == DEST_MCD)) {
            pad->dest[slot] = data;

            if ((data == DEST_JOY) && !joy)
                return;

            if ((data == DEST_MCD) && !mcd)
                return;

            if (pad->ctrl & CTRL_ACIE)
                pad->cycles_until_irq = JOY_IRQ_DELAY;
        }
    } else {
        switch (pad->dest[slot]) {
            case DEST_JOY: {
                if (!joy) {
                    pad->dest[slot] = 0;

                    return;
                }

                joy->write_func(joy->udata, data);

                if (!joy->query_fifo_func(joy->udata))
                    pad->dest[slot] = 0;
            } break;

            case DEST_MCD: {
                if (!mcd) {
                    pad->dest[slot] = 0;

                    return;
                }

                psx_mcd_write(mcd, data);

                if (pad->ctrl & CTRL_ACIE) {
                    pad->irq_bit = 1;
                    pad->cycles_until_irq = 1024;

                    return;
                }

                if (!psx_mcd_query(mcd))
                    pad->dest[slot] = 0;
            } break;
        }

        if (pad->ctrl & CTRL_ACIE) {
            pad->irq_bit = 1;
            pad->cycles_until_irq = (pad->dest[slot] == DEST_MCD) ? 2048 : JOY_IRQ_DELAY;
        }
    }
}

uint32_t pad_handle_stat_read(psx_pad_t* pad) {
    return pad->stat | 7;
}

void pad_handle_ctrl_write(psx_pad_t* pad, uint32_t value) {
    int slot = pad->ctrl & CTRL_SLOT;

    pad->ctrl = value;

    if (!(pad->ctrl & CTRL_JOUT)) {
        pad->ctrl &= ~CTRL_SLOT;

        if (pad->mcd_slot[slot >> 13])
            psx_mcd_reset(pad->mcd_slot[slot >> 13]);
    }

    // Reset STAT bits 3, 4, 5, 9
    if (pad->ctrl & CTRL_ACKN) {
        pad->stat &= 0xfdc7;
        pad->ctrl &= ~CTRL_ACKN;
    }
}

psx_pad_t* psx_pad_create(void) {
    return (psx_pad_t*)malloc(sizeof(psx_pad_t));
}

void psx_pad_init(psx_pad_t* pad, psx_ic_t* ic) {
    memset(pad, 0, sizeof(psx_pad_t));

    pad->ic = ic;

    pad->io_base = PSX_PAD_BEGIN;
    pad->io_size = PSX_PAD_SIZE;

    pad->joy_slot[0] = NULL;
    pad->joy_slot[1] = NULL;
    pad->mcd_slot[0] = NULL;
    pad->mcd_slot[1] = NULL;
}

uint32_t psx_pad_read32(psx_pad_t* pad, uint32_t offset) {
    uint32_t v = 0;

    switch (offset) {
        case 0: v = pad_read_rx(pad); break;
        case 4: v = pad_handle_stat_read(pad); break;
        case 8: v = pad->mode; break;
        case 10: v = pad->ctrl; break;
        case 14: v = pad->baud; break;
    }

    return v;
}

uint16_t psx_pad_read16(psx_pad_t* pad, uint32_t offset) {
    uint32_t v = 0;

    switch (offset) {
        case 0: v = pad_read_rx(pad) & 0xffff; break;
        case 4: v = pad_handle_stat_read(pad) & 0xffff; break;
        case 8: v = pad->mode; break;
        case 10: v = pad->ctrl & 0xffff; break;
        case 14: v = pad->baud; break;
    }

    return v;

    printf("Unhandled 16-bit PAD read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_pad_read8(psx_pad_t* pad, uint32_t offset) {
    uint32_t v = 0;

    switch (offset) {
        case 0: v = pad_read_rx(pad) & 0xff; break;
        case 4: v = pad_handle_stat_read(pad) & 0xff; break;
        case 8: v = pad->mode; break;
        case 10: v = pad->ctrl & 0xff; break;
        case 14: v = pad->baud; break;
    }

    // if (offset <= 0xa)
    // printf("slot=%d dest=%02x pad_read8(%02x)   -> %02x\n",
    //     (pad->ctrl & CTRL_SLOT) >> 13,
    //     pad->dest[(pad->ctrl & CTRL_SLOT) >> 13],
    //     offset,
    //     v
    // );

    return v;
}

void psx_pad_write32(psx_pad_t* pad, uint32_t offset, uint32_t value) {
    switch (offset) {
        case 0: pad_write_tx(pad, value); return;
        case 8: pad->mode = value & 0xffff; return;
        case 10: pad_handle_ctrl_write(pad, value); return;
        case 14: pad->baud = value & 0xffff; return;
    }

    printf("Unhandled 32-bit PAD write at offset %08x (%08x)", offset, value);
}

void psx_pad_write16(psx_pad_t* pad, uint32_t offset, uint16_t value) {
    switch (offset) {
        case 0: pad_write_tx(pad, value); return;
        case 8: pad->mode = value; return;
        case 10: pad_handle_ctrl_write(pad, value); return;
        case 14: pad->baud = value; return;
    }

    printf("Unhandled 16-bit PAD write at offset %08x (%04x)", offset, value);
}

void psx_pad_write8(psx_pad_t* pad, uint32_t offset, uint8_t value) {
    // if (offset <= 0xa)
    // printf("slot=%d dest=%02x pad_write8(%02x)  -> %02x\n",
    //     (pad->ctrl & CTRL_SLOT) >> 13,
    //     pad->dest[(pad->ctrl & CTRL_SLOT) >> 13],
    //     offset,
    //     value
    // );

    switch (offset) {
        case 0: pad_write_tx(pad, value); return;
        case 8: pad->mode = value; return;
        case 10: pad_handle_ctrl_write(pad, value); return;
        case 14: pad->baud = value; return;
    }

    printf("Unhandled 8-bit PAD write at offset %08x (%02x)", offset, value);
}

void psx_pad_button_press(psx_pad_t* pad, int slot, uint32_t data) {
    psx_input_t* selected_slot = pad->joy_slot[slot];

    if (selected_slot)
        selected_slot->on_button_press_func(selected_slot->udata, data);
}

void psx_pad_button_release(psx_pad_t* pad, int slot, uint32_t data) {
    psx_input_t* selected_slot = pad->joy_slot[slot];

    if (selected_slot)
        selected_slot->on_button_release_func(selected_slot->udata, data);
}

void psx_pad_analog_change(psx_pad_t* pad, int slot, uint32_t stick, uint16_t data) {
    psx_input_t* selected_slot = pad->joy_slot[slot];

    if (selected_slot)
        selected_slot->on_analog_change_func(selected_slot->udata, stick, data);
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

int psx_pad_attach_mcd(psx_pad_t* pad, int slot, const char* path) {
    printf("Memory Card support is disabled\n");

    return 0;

    if (pad->mcd_slot[slot])
        psx_pad_detach_mcd(pad, slot);

    psx_mcd_t* mcd = psx_mcd_create();

    int r = psx_mcd_init(mcd, path);
    
    if (r) {
        psx_pad_detach_mcd(pad, slot);

        return r;
    }

    pad->mcd_slot[slot] = mcd;

    return 0;
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

            if (pad->irq_bit) {
                pad->stat |= STAT_IRQ7;
                pad->irq_bit = 0;
            }

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