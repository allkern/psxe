#include "psx/bios.h"
#include "psx/ram.h"
#include "psx/dma.h"
#include "psx/exp1.h"
#include "psx/mc1.h"
#include "psx/mc2.h"
#include "psx/cpu.h"
#include "psx/log.h"

int main() {
    log_set_level(LOG_ERROR);

    psx_bios_t* bios = psx_bios_create();
    psx_ram_t* ram = psx_ram_create();
    psx_dma_t* dma = psx_dma_create();
    psx_exp1_t* exp1 = psx_exp1_create();
    psx_mc1_t* mc1 = psx_mc1_create();
    psx_mc2_t* mc2 = psx_mc2_create();
    psx_mc3_t* mc3 = psx_mc3_create();
    psx_ic_t* ic = psx_ic_create();
    psx_scratchpad_t* scratchpad = psx_scratchpad_create();

    // Bus initialization
    psx_bus_t* bus = psx_bus_create();

    psx_bus_init(bus);

    psx_bus_set_bios(bus, bios);
    psx_bus_set_ram(bus, ram);
    psx_bus_set_dma(bus, dma);
    psx_bus_set_exp1(bus, exp1);
    psx_bus_set_mc1(bus, mc1);
    psx_bus_set_mc2(bus, mc2);
    psx_bus_set_mc3(bus, mc3);
    psx_bus_set_ic(bus, ic);
    psx_bus_set_scratchpad(bus, scratchpad);

    // Init devices
    psx_bios_init(bios);
    psx_ram_init(ram, mc2);
    psx_dma_init(dma);
    psx_exp1_init(exp1, mc1);
    psx_mc1_init(mc1);
    psx_mc2_init(mc2);
    psx_mc3_init(mc3);
    psx_ic_init(ic);
    psx_scratchpad_init(scratchpad);

    psx_bios_load(bios, "scph1001.bin");

    // CPU creation and init
    psx_cpu_t* cpu = psx_cpu_create();

    psx_cpu_init(cpu, bus);

    while (true) {
        psx_cpu_cycle(cpu);
    }

    psx_cpu_destroy(cpu);
    psx_bios_destroy(bios);
    psx_bus_destroy(bus);
    psx_ram_destroy(ram);
    psx_exp1_destroy(exp1);
    psx_mc1_destroy(mc1);
    psx_mc2_destroy(mc2);
    psx_mc3_destroy(mc3);
    psx_ic_destroy(ic);
    psx_scratchpad_destroy(scratchpad);

    return 0;
}