#ifndef RAM_H
#define RAM_H

#include <stdint.h>

#include "log.h"

#define PSX_RAM_SIZE    0x200000
#define PSX_RAM_BEGIN   0x00000000
#define PSX_RAM_END     0x001fffff

typedef struct {
    uint8_t* buf;
} psx_ram_t;

psx_ram_t* psx_ram_create();
void psx_ram_init(psx_ram_t*);
uint32_t psx_ram_read32(psx_ram_t*, uint32_t);
uint16_t psx_ram_read16(psx_ram_t*, uint32_t);
uint8_t psx_ram_read8(psx_ram_t*, uint32_t);
void psx_ram_write32(psx_ram_t*, uint32_t, uint32_t);
void psx_ram_write16(psx_ram_t*, uint32_t, uint16_t);
void psx_ram_write8(psx_ram_t*, uint32_t, uint8_t);
void psx_ram_destroy(psx_ram_t*);

#endif