#ifndef PSX_H
#define PSX_H

#include "psx/cpu.h"
#include "psx/log.h"
#include "psx/exe.h"

#include <stdint.h>

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
    psx_gpu_t* gpu;
    psx_spu_t* spu;
    psx_bus_t* bus;
    psx_cpu_t* cpu;
    psx_timer_t* timer;
    psx_cdrom_t* cdrom;
} psx_t;

psx_t* psx_create();
void psx_init(psx_t*, const char*);
void psx_load_bios(psx_t*, const char*);
void psx_hard_reset(psx_t*);
void psx_soft_reset(psx_t*);
void psx_load_state(psx_t*, const char*);
void psx_save_state(psx_t*, const char*);
void psx_load_exe(psx_t*, const char*);
void psx_update(psx_t*);
void psx_run_frame(psx_t*);
uint32_t* psx_take_screenshot(psx_t*);
psx_bios_t* psx_get_bios(psx_t*);
psx_ram_t* psx_get_ram(psx_t*);
psx_dma_t* psx_get_dma(psx_t*);
psx_exp1_t* psx_get_exp1(psx_t*);
psx_mc1_t* psx_get_mc1(psx_t*);
psx_mc2_t* psx_get_mc2(psx_t*);
psx_mc3_t* psx_get_mc3(psx_t*);
psx_ic_t* psx_get_ic(psx_t*);
psx_scratchpad_t* psx_get_scratchpad(psx_t*);
psx_gpu_t* psx_get_gpu(psx_t*);
psx_spu_t* psx_get_spu(psx_t*);
psx_bus_t* psx_get_bus(psx_t*);
psx_timer_t* psx_get_timer(psx_t*);
psx_cdrom_t* psx_get_cdrom(psx_t*);
psx_cpu_t* psx_get_cpu(psx_t*);
void psx_destroy(psx_t*);

#endif