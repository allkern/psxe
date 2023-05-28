#ifndef BUS_H
#define BUS_H

#include <stdint.h>

#include "bios.h"
#include "ram.h"
#include "dma.h"
#include "exp1.h"
#include "mc1.h"
#include "mc2.h"
#include "mc3.h"
#include "ic.h"
#include "scratchpad.h"

typedef struct {
    psx_bios_t* bios;
    psx_ram_t* ram;
    psx_dma_t* dma;
    psx_exp1_t* exp1;
    psx_mc1_t* mc1;
    psx_mc2_t* mc2;
    psx_mc3_t* mc3;
    psx_ic_t* ic;
    psx_scratchpad_t* scratchpad;
} psx_bus_t;

#define PSX_GPUSTAT 0x1f801814

psx_bus_t* psx_bus_create();
void psx_bus_init(psx_bus_t*);
uint32_t psx_bus_read32(psx_bus_t*, uint32_t);
uint16_t psx_bus_read16(psx_bus_t*, uint32_t);
uint8_t psx_bus_read8(psx_bus_t*, uint32_t);
void psx_bus_write32(psx_bus_t*, uint32_t, uint32_t);
void psx_bus_write16(psx_bus_t*, uint32_t, uint16_t);
void psx_bus_write8(psx_bus_t*, uint32_t, uint8_t);
void psx_bus_set_bios(psx_bus_t*, psx_bios_t*);
void psx_bus_set_ram(psx_bus_t*, psx_ram_t*);
void psx_bus_set_dma(psx_bus_t*, psx_dma_t*);
void psx_bus_set_exp1(psx_bus_t*, psx_exp1_t*);
void psx_bus_set_mc1(psx_bus_t*, psx_mc1_t*);
void psx_bus_set_mc2(psx_bus_t*, psx_mc2_t*);
void psx_bus_set_mc3(psx_bus_t*, psx_mc3_t*);
void psx_bus_set_ic(psx_bus_t*, psx_ic_t*);
void psx_bus_set_scratchpad(psx_bus_t*, psx_scratchpad_t*);
void psx_bus_destroy(psx_bus_t*);

#endif