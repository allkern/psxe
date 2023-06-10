#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"
#include "../log.h"

psx_timer_t* psx_timer_create() {
    return (psx_timer_t*)malloc(sizeof(psx_timer_t));
}

/*
  FFFE0130h 4        Cache Control
*/
void psx_timer_init(psx_timer_t* timer) {
    memset(timer, 0, sizeof(psx_timer_t));

    timer->io_base = PSX_TIMER_BEGIN;
    timer->io_size = PSX_TIMER_SIZE;
}

uint32_t psx_timer_read32(psx_timer_t* timer, uint32_t offset) {
    log_warn("Unhandled 32-bit TIMER read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_timer_read16(psx_timer_t* timer, uint32_t offset) {
    log_warn("Unhandled 16-bit TIMER read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_timer_read8(psx_timer_t* timer, uint32_t offset) {
    log_warn("Unhandled 8-bit TIMER read at offset %08x", offset);

    return 0x0;
}

void psx_timer_write32(psx_timer_t* timer, uint32_t offset, uint32_t value) {
    log_warn("Unhandled 32-bit TIMER write at offset %08x (%08x)", offset, value);
}

void psx_timer_write16(psx_timer_t* timer, uint32_t offset, uint16_t value) {
    log_warn("Unhandled 16-bit TIMER write at offset %08x (%04x)", offset, value);
}

void psx_timer_write8(psx_timer_t* timer, uint32_t offset, uint8_t value) {
    log_warn("Unhandled 8-bit TIMER write at offset %08x (%02x)", offset, value);
}

void psx_timer_destroy(psx_timer_t* timer) {
    free(timer);
}