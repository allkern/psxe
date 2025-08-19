#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../log.h"
#include "scratchpad.h"
#include "../mem_track.h"

// Static buffer for scratchpad instance
static psx_scratchpad_t g_scratchpad_instance;
static uint8_t g_scratchpad_buffer[PSX_SCRATCHPAD_SIZE];
static int g_scratchpad_instance_used = 0;

psx_scratchpad_t* psx_scratchpad_create(void) {
    if (g_scratchpad_instance_used) {
        return NULL; // Only one instance allowed
    }
    g_scratchpad_instance_used = 1;
    
    // Register static buffer sizes
    REGISTER_STATIC_BUFFER(g_scratchpad_instance, "scratchpad_instance");
    REGISTER_STATIC_BUFFER(g_scratchpad_buffer, "scratchpad_buffer");
    
    return &g_scratchpad_instance;
}

void psx_scratchpad_init(psx_scratchpad_t* scratchpad) {
    memset(scratchpad, 0, sizeof(psx_scratchpad_t));

    scratchpad->io_base = PSX_SCRATCHPAD_BEGIN;
    scratchpad->io_size = PSX_SCRATCHPAD_SIZE;

    scratchpad->buf = g_scratchpad_buffer;
}

uint32_t psx_scratchpad_read32(psx_scratchpad_t* scratchpad, uint32_t offset) {
    return *((uint32_t*)(scratchpad->buf + offset));
}

uint16_t psx_scratchpad_read16(psx_scratchpad_t* scratchpad, uint32_t offset) {
    return *((uint16_t*)(scratchpad->buf + offset));
}

uint8_t psx_scratchpad_read8(psx_scratchpad_t* scratchpad, uint32_t offset) {
    return scratchpad->buf[offset];
}

void psx_scratchpad_write32(psx_scratchpad_t* scratchpad, uint32_t offset, uint32_t value) {
    *((uint32_t*)(scratchpad->buf + offset)) = value;
}

void psx_scratchpad_write16(psx_scratchpad_t* scratchpad, uint32_t offset, uint16_t value) {
    *((uint16_t*)(scratchpad->buf + offset)) = value;
}

void psx_scratchpad_write8(psx_scratchpad_t* scratchpad, uint32_t offset, uint8_t value) {
    scratchpad->buf[offset] = value;
}

void psx_scratchpad_destroy(psx_scratchpad_t* scratchpad) {
    // Mark instance as available again
    g_scratchpad_instance_used = 0;
}