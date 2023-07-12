#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#include "ic.h"
#include "gpu.h"

#define PSX_TIMER_BEGIN 0x1f801100
#define PSX_TIMER_SIZE  0x30
#define PSX_TIMER_END   0x1f80112f

typedef struct {
    uint32_t io_base, io_size;

    psx_ic_t* ic;

    struct {
        uint32_t counter;
        uint32_t mode;
        uint32_t target;
        uint32_t invalid;
    } timer[3];
} psx_timer_t;

psx_timer_t* psx_timer_create();
void psx_timer_init(psx_timer_t*, psx_ic_t*);
uint32_t psx_timer_read32(psx_timer_t*, uint32_t);
uint16_t psx_timer_read16(psx_timer_t*, uint32_t);
uint8_t psx_timer_read8(psx_timer_t*, uint32_t);
void psx_timer_write32(psx_timer_t*, uint32_t, uint32_t);
void psx_timer_write16(psx_timer_t*, uint32_t, uint16_t);
void psx_timer_write8(psx_timer_t*, uint32_t, uint8_t);
void psx_timer_update(psx_timer_t*);
void psx_timer_destroy(psx_timer_t*);

// Hblank event handler
void psxe_gpu_hblank_event_cb(psx_gpu_t*);

#endif