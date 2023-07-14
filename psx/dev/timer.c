#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"
#include "../log.h"

#define T0_COUNTER timer->timer[0].counter
#define T0_PREV timer->timer[0].prev_counter
#define T0_MODE timer->timer[0].mode
#define T0_TARGET timer->timer[0].target
#define T0_PAUSED timer->timer[0].paused
#define T1_COUNTER timer->timer[1].counter
#define T1_PREV timer->timer[1].prev_counter
#define T1_MODE timer->timer[1].mode
#define T1_TARGET timer->timer[1].target
#define T1_PAUSED timer->timer[1].paused
#define T2_COUNTER timer->timer[2].counter
#define T2_PREV timer->timer[2].prev_counter
#define T2_MODE timer->timer[2].mode
#define T2_TARGET timer->timer[2].target
#define T2_PAUSED timer->timer[2].paused

psx_timer_t* psx_timer_create() {
    return (psx_timer_t*)malloc(sizeof(psx_timer_t));
}

void psx_timer_init(psx_timer_t* timer, psx_ic_t* ic) {
    memset(timer, 0, sizeof(psx_timer_t));

    timer->io_base = PSX_TIMER_BEGIN;
    timer->io_size = PSX_TIMER_SIZE;

    timer->ic = ic;
}

uint32_t psx_timer_read32(psx_timer_t* timer, uint32_t offset) {
    int index = offset >> 4;
    int reg = offset & 0xf;

    switch (reg) {
        case 0: return timer->timer[index].counter;
        case 4: return timer->timer[index].mode;
        case 8: return timer->timer[index].target;
    }

    log_fatal("Unhandled 32-bit TIMER read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_timer_read16(psx_timer_t* timer, uint32_t offset) {
    int index = offset >> 4;
    int reg = offset & 0xf;

    switch (reg) {
        case 0: return timer->timer[index].counter;
        case 4: return timer->timer[index].mode;
        case 8: return timer->timer[index].target;
    }

    log_fatal("Unhandled 16-bit TIMER read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_timer_read8(psx_timer_t* timer, uint32_t offset) {
    log_fatal("Unhandled 8-bit TIMER read at offset %08x", offset);

    return 0x0;
}

void psx_timer_write32(psx_timer_t* timer, uint32_t offset, uint32_t value) {
    int index = offset >> 4;
    int reg = offset & 0xf;

    switch (reg) {
        case 0: {
            timer->timer[index].counter = value;
            timer->f_counter[index] = value;
        } return;
        case 4: timer->timer[index].mode = value; return;
        case 8: timer->timer[index].target = value; return;
    }

    log_fatal("Unhandled 32-bit TIMER write at offset %08x (%02x)", offset, value);
}

void psx_timer_write16(psx_timer_t* timer, uint32_t offset, uint16_t value) {
    int index = offset >> 4;
    int reg = offset & 0xf;

    switch (reg) {
        case 0: {
            timer->timer[index].counter = value;
            timer->f_counter[index] = value;
        } return;
        case 4: timer->timer[index].mode = value; return;
        case 8: timer->timer[index].target = value; return;
    }

    log_fatal("Unhandled 16-bit TIMER write at offset %08x (%02x)", offset, value);
}

void psx_timer_write8(psx_timer_t* timer, uint32_t offset, uint8_t value) {
    log_fatal("Unhandled 8-bit TIMER write at offset %08x (%02x)", offset, value);
}

void timer_update_timer0(psx_timer_t* timer, int cyc) {
    int src = T0_MODE & 0x100;

    // if (src) {
    //     log_fatal("Unimplemented T0 clock source: dotclock");

    //     exit(1);
    // }

    T0_PREV = T0_COUNTER;

    if (!T0_PAUSED)
        T0_COUNTER += cyc;

    uint16_t reset = (T0_MODE & MODE_RESETC) ? T0_TARGET : 0xffff;

    if ((T0_COUNTER >= reset) && (T0_PREV <= reset)) {
        T0_COUNTER -= reset;
        T0_MODE |= 0x800;

        if (reset == 0xffff)
            T0_MODE |= 0x1000;
    }

    if ((T0_COUNTER >= 0xffff) && (T0_PREV <= 0xffff)) {
        T0_COUNTER &= 0xffff;
        T0_MODE |= 0x1000;
    }
}

void timer_update_timer1(psx_timer_t* timer, int cyc) {
    int src = T1_MODE & 0x100;

    // if (src) {
    //     log_fatal("Unimplemented T1 clock source: dotclock");

    //     exit(1);
    // }

    T1_PREV = T1_COUNTER;

    if (!T1_PAUSED)
        T1_COUNTER += cyc;

    uint16_t reset = (T1_MODE & MODE_RESETC) ? T1_TARGET : 0xffff;

    if ((T1_COUNTER >= reset) && (T1_PREV <= reset)) {
        T1_COUNTER -= reset;
        T1_MODE |= 0x800;

        if (reset == 0xffff)
            T1_MODE |= 0x1000;
    }

    if ((T1_COUNTER >= 0xffff) && (T1_PREV <= 0xffff)) {
        T1_COUNTER &= 0xffff;
        T1_MODE |= 0x1000;
    }
}
void timer_update_timer2(psx_timer_t* timer, int cyc) {
    int src = T2_MODE & 0x100;

    // if (src) {
    //     log_fatal("Unimplemented T2 clock source: dotclock");

    //     exit(1);
    // }

    T2_PREV = T2_COUNTER;

    if (!T2_PAUSED)
        T2_COUNTER += cyc;

    uint16_t reset = (T2_MODE & MODE_RESETC) ? T2_TARGET : 0xffff;

    if ((T2_COUNTER >= reset) && (T2_PREV <= reset)) {
        T2_COUNTER -= reset;
        T2_MODE |= 0x800;

        if (reset == 0xffff)
            T2_MODE |= 0x1000;
    }

    if ((T2_COUNTER >= 0xffff) && (T2_PREV <= 0xffff)) {
        T2_COUNTER &= 0xffff;
        T2_MODE |= 0x1000;
    }
}

void psx_timer_update(psx_timer_t* timer, int cyc) {
    timer_update_timer0(timer, cyc);
    timer_update_timer1(timer, cyc);
    timer_update_timer2(timer, cyc);
}

void psxe_gpu_hblank_event_cb(psx_gpu_t* gpu) {}
void psxe_gpu_hblank_end_event_cb(psx_gpu_t* gpu) {}
void psxe_gpu_vblank_end_event_cb(psx_gpu_t* gpu) {}

void psx_timer_destroy(psx_timer_t* timer) {
    free(timer);
}