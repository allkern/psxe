#ifndef BUS_H
#define BUS_H

#include <stdint.h>

#include "bios.h"
#include "ram.h"

typedef struct {
    psx_bios_t* bios;
    psx_ram_t* ram;

    // Unhandled I/O ports
    uint32_t ram_size;
} psx_bus_t;

#define PSX_IO_RAM_SIZE_BEGIN 0x1f801060
#define PSX_IO_RAM_SIZE_SIZE  0x4
#define PSX_IO_RAM_SIZE_END   0x1f801063
#define PSX_EXP1_ROM_BEGIN    0x1f000000
#define PSX_EXP1_ROM_SIZE     0x100
#define PSX_EXP1_ROM_END      0x1f0000ff

psx_bus_t* psx_bus_create();
void psx_bus_init(psx_bus_t*, psx_bios_t*, psx_ram_t*);
void psx_bus_destroy(psx_bus_t*);
uint32_t psx_bus_read32(psx_bus_t*, uint32_t);
uint16_t psx_bus_read16(psx_bus_t*, uint32_t);
uint8_t psx_bus_read8(psx_bus_t*, uint32_t);
void psx_bus_write32(psx_bus_t*, uint32_t, uint32_t);
void psx_bus_write16(psx_bus_t*, uint32_t, uint16_t);
void psx_bus_write8(psx_bus_t*, uint32_t, uint8_t);

#endif