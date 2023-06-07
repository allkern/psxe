#include "screen.h"

psxe_screen_t* psxe_screen_create() {
    return (psxe_screen_t*)malloc(sizeof(psxe_screen_t));
}

void psxe_screen_init(psxe_screen_t* screen, psx_gpu_t* gpu) {
    memset(screen, 0, sizeof(psxe_screen_t));

    screen->width = 320;
    screen->height = 240;
    screen->scale = 1;
    screen->format = SDL_PIXELFORMAT_BGR555;
    screen->mode = 60;
    screen->buf = (uint32_t*)gpu->vram;
    screen->open = 1;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
}

void psxe_screen_reload(psxe_screen_t* screen) {
    if (screen->texture) SDL_DestroyTexture(screen->texture);
    if (screen->renderer) SDL_DestroyRenderer(screen->renderer);
    if (screen->window) SDL_DestroyWindow(screen->window);

    screen->window = SDL_CreateWindow(
        "PSX",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screen->width * screen->scale,
        screen->height * screen->scale,
        SDL_WINDOW_OPENGL
    );

    screen->renderer = SDL_CreateRenderer(
        screen->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    screen->texture = SDL_CreateTexture(
        screen->renderer,
        screen->format,
        SDL_TEXTUREACCESS_STREAMING,
        PSX_GPU_FB_WIDTH, PSX_GPU_FB_HEIGHT
    );

    screen->open = 1;
}

int psxe_screen_is_open(psxe_screen_t* screen) {
    return screen->open;
}

void psxe_screen_update(psxe_screen_t* screen) {
    SDL_UpdateTexture(screen->texture, NULL, screen->buf, PSX_GPU_FB_WIDTH * sizeof(uint16_t));
    SDL_RenderCopy(screen->renderer, screen->texture, NULL, NULL);
    SDL_RenderPresent(screen->renderer);

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                screen->open = 0;
            } break;
        }
    }
}

void psxe_screen_set_scale(psxe_screen_t* screen, unsigned int scale) {
    screen->scale = scale;
}

void psxe_screen_destroy(psxe_screen_t* screen) {
    SDL_DestroyTexture(screen->texture);
    SDL_DestroyRenderer(screen->renderer);
    SDL_DestroyWindow(screen->window);

    SDL_Quit();

    free(screen);
}

void psxe_gpu_dmode_event_cb(psx_gpu_t* gpu) {
    psxe_screen_t* screen = gpu->udata[0];

    static int dmode_hres_table[] = {
        256, 320, 512, 640
    };

    screen->width = dmode_hres_table[gpu->display_mode & 0x3];
    screen->height = (gpu->display_mode & 0x4) ? 480 : 240;
    screen->format = gpu->display_mode & 0x10 ? SDL_PIXELFORMAT_BGR888 : SDL_PIXELFORMAT_BGR555;
    screen->mode = gpu->display_mode & 0x8 ? 60 : 50;

    if (screen->width >= 512)
        screen->scale = 1;

    SDL_DestroyTexture(screen->texture);

    screen->texture = SDL_CreateTexture(
        screen->renderer,
        screen->format,
        SDL_TEXTUREACCESS_STREAMING,
        screen->width, screen->height
    );

    SDL_SetWindowSize(
        screen->window,
        screen->width * screen->scale,
        screen->height * screen->scale
    );
}