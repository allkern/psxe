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
#define T0_IRQ_FIRED timer->timer[0].irq_fired

#define T1_COUNTER timer->timer[1].counter
#define T1_PREV timer->timer[1].prev_counter
#define T1_MODE timer->timer[1].mode
#define T1_TARGET timer->timer[1].target
#define T1_PAUSED timer->timer[1].paused
#define T1_IRQ_FIRED timer->timer[1].irq_fired

#define T2_COUNTER timer->timer[2].counter
#define T2_PREV timer->timer[2].prev_counter
#define T2_MODE timer->timer[2].mode
#define T2_TARGET timer->timer[2].target
#define T2_PAUSED timer->timer[2].paused
#define T2_IRQ_FIRED timer->timer[2].irq_fired

// bool should_I_pause_the_timer(psx_timer_t* timer) {
//   if ((timer->mode & 1) == 0) return false;
//   switch ((timer->mode >> 1) & 3) {
//     case 0: return gpu.isXblank();
//     case 1: return false;
//     case 2: return !gpu.isXblank();
//     case 3: return gpu.gotXblankOnce();
//   }
// }

// bool did_timer_reach_target(Timer timer) {
//   if ((timer.mode & 8) == 1) return timer.value >= timer.target;
//   return timer.value >= 0xffff;
// }

// bool should_I_reset_the_timer(Timer timer) {
//   if (did_timer_reach_target(timer)) return true;
//   if ((timer.mode & 1) == 0) return false;
//   switch ((timer.mode >> 1) & 3) {
//     case 1:
//     case 2:
//       return gpu.isXBlank();
//   }
//   return false;
// }

const char* g_psx_timer_reg_names[] = {
    "counter", 0, 0, 0,
    "mode", 0, 0, 0,
    "target", 0, 0, 0
};

psx_timer_t* psx_timer_create(void) {
    return (psx_timer_t*)malloc(sizeof(psx_timer_t));
}

void psx_timer_init(psx_timer_t* timer, psx_ic_t* ic) {
    memset(timer, 0, sizeof(psx_timer_t));

    timer->io_base = PSX_TIMER_BEGIN;
    timer->io_size = PSX_TIMER_SIZE;

    timer->ic = ic;
}

int t1_counter = 0;

uint32_t psx_timer_read32(psx_timer_t* timer, uint32_t offset) {
    int index = offset >> 4;
    int reg = offset & 0xf;

    switch (reg) {
        case 0: {
            return timer->timer[index].counter;
        } break;
        case 4: {
            timer->timer[index].mode &= 0xffffe7ff;

            return timer->timer[index].mode;
        } break;
        case 8: return timer->timer[index].target;
    }

    log_fatal("Unhandled 32-bit TIMER read at offset %08x", offset);

    // exit(1);

    return 0x0;
}

uint16_t psx_timer_read16(psx_timer_t* timer, uint32_t offset) {
    int index = offset >> 4;
    int reg = offset & 0xf;

    switch (reg) {
        case 0: return timer->timer[index].counter;
        case 4: {
            timer->timer[index].mode &= 0xffffe7ff;

            return timer->timer[index].mode;
        } break;
        case 8: return timer->timer[index].target;
    }

    printf("Unhandled 16-bit TIMER read at offset %08x\n", offset);

    return 0x0;
}

uint8_t psx_timer_read8(psx_timer_t* timer, uint32_t offset) {
    printf("Unhandled 8-bit TIMER read at offset %08x\n", offset);

    return 0x0;
}

void psx_timer_write32(psx_timer_t* timer, uint32_t offset, uint32_t value) {
    int index = offset >> 4;
    int reg = offset & 0xf;

    switch (reg) {
        case 0: {
            timer->timer[index].counter = value;
        } return;
        case 4: {
            timer->timer[index].mode = value;
            timer->timer[index].mode |= 0x400;
            timer->timer[index].irq_fired = 0;
            timer->timer[index].counter = 0;
        } return;
        case 8: timer->timer[index].target = value; return;
    }

    log_fatal("Unhandled 32-bit TIMER write at offset %08x (%02x)", offset, value);

    // exit(1);
}

void psx_timer_write16(psx_timer_t* timer, uint32_t offset, uint16_t value) {
    int index = offset >> 4;
    int reg = offset & 0xf;

    switch (reg) {
        case 0: {
            timer->timer[index].counter = value;
        } return;
        case 4: {
            timer->timer[index].mode = value;
            timer->timer[index].mode |= 0x400;
            timer->timer[index].irq_fired = 0;
            timer->timer[index].counter = 0;
        } return;
        case 8: {
            timer->timer[index].target = value;
        } return;
    }

    printf("Unhandled 16-bit TIMER write at offset %08x (%02x)\n", offset, value);

    // exit(1);
}

void psx_timer_write8(psx_timer_t* timer, uint32_t offset, uint8_t value) {
    printf("Unhandled 8-bit TIMER write at offset %08x (%02x)\n", offset, value);
}

void timer_update_timer0(psx_timer_t* timer, int cyc) {
    int reached_target = ((uint32_t)T0_COUNTER + cyc) >= T0_TARGET;
    int reached_max = ((uint32_t)T0_COUNTER + cyc) >= 0xffff;

    // Dotclock unsupported
    // if (T0_MODE & 0x100)

    if (!T0_PAUSED)
        T0_COUNTER += cyc;

    int can_fire_irq = (T0_MODE & MODE_IRQRMD) || !T0_IRQ_FIRED;

    int target_irq = reached_target && (T0_MODE & MODE_TGTIRQ);
    int max_irq = reached_max && (T0_MODE & MODE_MAXIRQ);

    T0_MODE &= ~0x0800;
    T0_MODE |= reached_target << 11;

    T0_MODE &= ~0x1000;
    T0_MODE |= reached_max << 12;

    if ((target_irq || max_irq) && can_fire_irq) {
        if (T0_MODE & MODE_IRQPMD) {
            T0_MODE ^= 0x400;
        } else {
            T0_MODE |= 0x400;
        }

        timer->timer[0].irq_fired = 1;

        psx_ic_irq(timer->ic, IC_TIMER0);
    }

    if (T0_MODE & MODE_RESETC) {
        if (reached_target)
            T0_COUNTER -= T0_TARGET;
    }
}

void timer_update_timer1(psx_timer_t* timer, int cyc) {
    int reached_target, reached_max;

    if (T1_MODE & 0x100) {
        reached_target = T1_COUNTER == T1_TARGET;
        reached_max = T1_COUNTER == 0xffff;
    } else {
        reached_target = ((uint32_t)T1_COUNTER + cyc) >= T1_TARGET;
        reached_max = ((uint32_t)T1_COUNTER + cyc) >= 0xffff;

        if (!T1_PAUSED)
            T1_COUNTER += cyc;
    }
    
    int can_fire_irq = (T1_MODE & MODE_IRQRMD) || !T1_IRQ_FIRED;

    int target_irq = reached_target && (T1_MODE & MODE_TGTIRQ);
    int max_irq = reached_max && (T1_MODE & MODE_MAXIRQ);

    T1_MODE &= ~0x0800;
    T1_MODE |= reached_target << 11;

    T1_MODE &= ~0x1000;
    T1_MODE |= reached_max << 12;

    if ((target_irq || max_irq) && can_fire_irq) {
        if (T1_MODE & MODE_IRQPMD) {
            T1_MODE ^= 0x400;
        } else {
            T1_MODE |= 0x400;
        }

        T1_IRQ_FIRED = 1;

        psx_ic_irq(timer->ic, IC_TIMER1);
    }

    if (T1_MODE & MODE_RESETC) {
        if (reached_target)
            T1_COUNTER -= T1_TARGET;
    }
}

void timer_update_timer2(psx_timer_t* timer, int cyc) {
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

void psxe_gpu_hblank_event_cb(psx_gpu_t* gpu) {
    psx_timer_t* timer = gpu->udata[1];

    if (T1_MODE & 0x100 && !T1_PAUSED)
        T1_COUNTER++;

    if (T0_MODE & MODE_SYNCEN) {
        switch (T0_MODE & 6) {
            case 0: {
                T0_PAUSED = 1;
            } break;

            case 2: {
                T0_COUNTER = 0;
            } break;

            case 4: {
                T0_COUNTER = 0;
                T0_PAUSED = 0;
            } break;

            case 6: {
                T0_MODE &= ~MODE_SYNCEN;
            } break;
        }
    }
}

void psxe_gpu_hblank_end_event_cb(psx_gpu_t* gpu) {
    psx_timer_t* timer = gpu->udata[1];

    if (T0_MODE & MODE_SYNCEN) {
        switch (T0_MODE & 6) {
            case 0: {
                T0_PAUSED = 0;
            } break;

            case 4: {
                T0_PAUSED = 1;
            } break;
        }
    }
}

void psxe_gpu_vblank_timer_event_cb(psx_gpu_t* gpu) {
    psx_timer_t* timer = gpu->udata[1];

    if (T1_MODE & MODE_SYNCEN) {
        switch (T1_MODE & 6) {
            case 0: {
                T1_PAUSED = 1;
            } break;

            case 2: {
                T1_COUNTER = 0;
            } break;

            case 4: {
                T1_COUNTER = 0;
                T1_PAUSED = 0;
            } break;

            case 6: {
                T1_MODE &= ~MODE_SYNCEN;
            } break;
        }
    }
}

void psxe_gpu_vblank_end_event_cb(psx_gpu_t* gpu) {
    psx_timer_t* timer = gpu->udata[1];

    if (T1_MODE & MODE_SYNCEN) {
        switch (T1_MODE & 6) {
            case 0: {
                T1_PAUSED = 0;
            } break;

            case 4: {
                T1_PAUSED = 1;
            } break;
        }
    }
}

void psx_timer_destroy(psx_timer_t* timer) {
    free(timer);
}