#include "psx/bios.h"
#include "psx/cpu.h"
#include "psx/log.h"

int main() {
    log_set_level(LOG_WARN);

    psx_bios_t* bios = psx_bios_create();

    psx_bios_init(bios);
    psx_bios_load(bios, "scph1001.bin");

    psx_ram_t* ram = psx_ram_create();

    psx_ram_init(ram);

    psx_bus_t* bus = psx_bus_create();
    
    psx_bus_init(bus, bios, ram);

    psx_cpu_t* cpu = psx_cpu_create();

    psx_cpu_init(cpu, bus);

    while (true) {
        psx_cpu_cycle(cpu);
    }

    psx_cpu_destroy(cpu);
    psx_bios_destroy(bios);
    psx_bus_destroy(bus);
    psx_ram_destroy(ram);
}