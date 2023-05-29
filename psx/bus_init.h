#ifndef BUS_INIT_H
#define BUS_INIT_H

#include "bios.h"
#include "ram.h"
#include "dma.h"
#include "exp1.h"
#include "mc1.h"
#include "mc2.h"
#include "mc3.h"
#include "ic.h"
#include "scratchpad.h"
#include "gpu.h"
#include "spu.h"

struct psx_bus_t {
    psx_bios_t* bios;
    psx_ram_t* ram;
    psx_dma_t* dma;
    psx_exp1_t* exp1;
    psx_mc1_t* mc1;
    psx_mc2_t* mc2;
    psx_mc3_t* mc3;
    psx_ic_t* ic;
    psx_scratchpad_t* scratchpad;
    psx_gpu_t* gpu;
    psx_spu_t* spu;
};

void psx_bus_init_bios(psx_bus_t*, psx_bios_t*);
void psx_bus_init_ram(psx_bus_t*, psx_ram_t*);
void psx_bus_init_dma(psx_bus_t*, psx_dma_t*);
void psx_bus_init_exp1(psx_bus_t*, psx_exp1_t*);
void psx_bus_init_mc1(psx_bus_t*, psx_mc1_t*);
void psx_bus_init_mc2(psx_bus_t*, psx_mc2_t*);
void psx_bus_init_mc3(psx_bus_t*, psx_mc3_t*);
void psx_bus_init_ic(psx_bus_t*, psx_ic_t*);
void psx_bus_init_scratchpad(psx_bus_t*, psx_scratchpad_t*);
void psx_bus_init_gpu(psx_bus_t*, psx_gpu_t*);
void psx_bus_init_spu(psx_bus_t*, psx_spu_t*);

#endif