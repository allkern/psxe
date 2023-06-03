#include "psx/bus.h"
#include "psx/bus_init.h"
#include "psx/cpu.h"
#include "psx/log.h"
#include "psx/exe.h"

#include "argparse.h"
#include "SDL.h"

#undef main

uint32_t g_psx_gpu_display_mode_hres1_table[] = {
    256, 320, 512, 640
};

uint32_t g_psx_gpu_display_mode_vres_table[] = {
    240, 480
};

typedef struct {
    int w, h, f, m, s;
} frontend_window_t;

void psx_dmode_event_cb(psx_gpu_t* gpu) {
    SDL_Window* window = gpu->udata[0];
    SDL_Renderer* renderer = gpu->udata[1];
    SDL_Texture** texture = gpu->udata[2];
    frontend_window_t* fe_data = gpu->udata[3];

    fe_data->w = g_psx_gpu_display_mode_hres1_table[gpu->display_mode & 0x3];
    fe_data->h = (gpu->display_mode & 0x4) ? 480 : 240;
    fe_data->f = gpu->display_mode & 0x10 ? SDL_PIXELFORMAT_BGR888 : SDL_PIXELFORMAT_BGR555;
    fe_data->m = gpu->display_mode & 0x8 ? 60 : 50;

    if (fe_data->w >= 512)
        fe_data->s = 1;

    SDL_DestroyTexture(*texture);

    *texture = SDL_CreateTexture(
        renderer,
        fe_data->f,
        SDL_TEXTUREACCESS_STREAMING,
        fe_data->w, fe_data->h
    );

    SDL_SetWindowSize(window, fe_data->w*fe_data->s, fe_data->h*fe_data->s);
}

int main(int argc, const char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    frontend_window_t fe_data;

    fe_data.w = 320;
    fe_data.h = 240;
    fe_data.f = SDL_PIXELFORMAT_BGR555;
    fe_data.m = 60;
    fe_data.s = 2;

    SDL_Window* window = SDL_CreateWindow(
        "PSX",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        fe_data.w*fe_data.s, fe_data.h*fe_data.s,
        SDL_WINDOW_OPENGL
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        fe_data.f,
        SDL_TEXTUREACCESS_STREAMING,
        fe_data.w, fe_data.h
    );

    log_set_level(LOG_FATAL);
    
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

    psx_gpu_set_dmode_event_callback(gpu, psx_dmode_event_cb);
    psx_gpu_set_udata(gpu, 0, window);
    psx_gpu_set_udata(gpu, 1, renderer);
    psx_gpu_set_udata(gpu, 2, &texture);
    psx_gpu_set_udata(gpu, 3, &fe_data);

    psx_bios_load(bios, "SCPH1001.bin");

    psx_cpu_t* cpu = psx_cpu_create();

    psx_cpu_init(cpu, bus);

    if (argv[1]) {
        psx_exe_load(cpu, argv[1]);
    }

    int open = 1;

    
    while (open) {
        // PSX CPU runs at 33.8688 MHz
        // Each instruction takes an average of 4 cycles
        // Instruction freq is cpuspeed/cycles
        // So the CPU runs cpuspeed/cycles/30 instructions
        // each Vblank
        unsigned int counter = (33868800 / 4) / fe_data.m;

        while (counter--) {
            psx_cpu_cycle(cpu);
        }

        SDL_UpdateTexture(texture, NULL, gpu->vram, 1024);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: { open = 0; } break;
            }
        }
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

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}