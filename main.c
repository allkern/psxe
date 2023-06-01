#include "psx/bus.h"
#include "psx/bus_init.h"
#include "psx/cpu.h"
#include "psx/log.h"

#include "getopt.h"

int main(int argc, const char* argv[]) {
    int log_level = LOG_WARN;
    const char* bios_path = "bios/scph1001.bin";

    static struct option long_options[] = {
        { "log-level"   , required_argument, 0, 'L' },
        { "bios"        , required_argument, 0, 'B' },
        { 0, 0, 0, 0 }
    };

    int option_index = 0;

    int c = getopt_long(
        argc, (char* const*)argv,
        "L:B:", long_options,
        &option_index
    );

    while (c != -1) {
        switch (c) {
            case 'L': {
                log_level = atoi(optarg);
            } break;

            case 'B': {
                bios_path = optarg;
            } break;

        }

        c = getopt_long(
            argc, (char* const*)argv,
            "L:B:", long_options,
            &option_index
        );
    }

    log_set_level(log_level);

    psx_bios_t* bios = psx_bios_create();
    psx_ram_t* ram = psx_ram_create();
    psx_dma_t* dma = psx_dma_create();
    psx_exp1_t* exp1 = psx_exp1_create();
    psx_mc1_t* mc1 = psx_mc1_create();
    psx_mc2_t* mc2 = psx_mc2_create();
    psx_mc3_t* mc3 = psx_mc3_create();
    psx_ic_t* ic = psx_ic_create();
    psx_scratchpad_t* scratchpad = psx_scratchpad_create();
    psx_gpu_t* gpu = psx_gpu_create();
    psx_spu_t* spu = psx_spu_create();

    // Bus initialization
    psx_bus_t* bus = psx_bus_create();

    psx_bus_init(bus);

    psx_bus_init_bios(bus, bios);
    psx_bus_init_ram(bus, ram);
    psx_bus_init_dma(bus, dma);
    psx_bus_init_exp1(bus, exp1);
    psx_bus_init_mc1(bus, mc1);
    psx_bus_init_mc2(bus, mc2);
    psx_bus_init_mc3(bus, mc3);
    psx_bus_init_ic(bus, ic);
    psx_bus_init_scratchpad(bus, scratchpad);
    psx_bus_init_gpu(bus, gpu);
    psx_bus_init_spu(bus, spu);

    // Init devices
    psx_bios_init(bios);
    psx_ram_init(ram, mc2);
    psx_dma_init(dma, bus);
    psx_exp1_init(exp1, mc1);
    psx_mc1_init(mc1);
    psx_mc2_init(mc2);
    psx_mc3_init(mc3);
    psx_ic_init(ic);
    psx_scratchpad_init(scratchpad);
    psx_gpu_init(gpu);
    psx_spu_init(spu);

    psx_bios_load(bios, bios_path);

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
    psx_gpu_destroy(gpu);
    psx_spu_destroy(spu);

    return 0;
}