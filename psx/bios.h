#ifndef BIOS_H
#define BIOS_H

#include <stdint.h>

#define PSXE_DEBUG_BIOS 1

#ifdef PSXE_DEBUG_BIOS
#include "log.h"
#endif

#define PSX_BIOS_SIZE   0x80000
#define PSX_BIOS_BEGIN  0xbfc00000
#define PSX_BIOS_END    0xbfc7ffff

typedef struct {
    uint8_t* buf;
} psx_bios_t;

psx_bios_t* psx_bios_create();
void psx_bios_init(psx_bios_t*);
void psx_bios_load(psx_bios_t*, const char*);
uint32_t psx_bios_read32(psx_bios_t*, uint32_t);
uint16_t psx_bios_read16(psx_bios_t*, uint32_t);
uint8_t psx_bios_read8(psx_bios_t*, uint32_t);
void psx_bios_destroy(psx_bios_t*);

#endif