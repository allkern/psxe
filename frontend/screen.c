#include "screen.h"

// #define PSXE_SCREEN_DEBUG 1

psxe_screen_t* psxe_screen_create() {
    return (psxe_screen_t*)malloc(sizeof(psxe_screen_t));
}

void psxe_screen_init(psxe_screen_t* screen, psx_t* psx) {
    memset(screen, 0, sizeof(psxe_screen_t));

#ifdef PSXE_SCREEN_DEBUG
    screen->width = PSX_GPU_FB_WIDTH;
    screen->height = PSX_GPU_FB_HEIGHT;
#else
    screen->width = 320;
    screen->height = 240;
#endif
    screen->scale = 1;
    screen->open = 1;
    screen->psx = psx;

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
        SDL_PIXELFORMAT_BGR555,
        SDL_TEXTUREACCESS_STREAMING,
        PSX_GPU_FB_WIDTH, PSX_GPU_FB_HEIGHT
    );

    screen->open = 1;
}

int psxe_screen_is_open(psxe_screen_t* screen) {
    return screen->open;
}

void psxe_screen_update(psxe_screen_t* screen) {
    void* display_buf = psx_get_display_buffer(screen->psx);

    SDL_UpdateTexture(screen->texture, NULL, display_buf, PSX_GPU_FB_STRIDE);
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
#ifdef PSXE_SCREEN_DEBUG
    screen->scale = 1;
#else
    screen->scale = scale;
#endif
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

#ifdef PSXE_SCREEN_DEBUG
    screen->width = PSX_GPU_FB_WIDTH; // dmode_hres_table[gpu->display_mode & 0x3];
    screen->height = PSX_GPU_FB_HEIGHT; // (gpu->display_mode & 0x4) ? 480 : 240;
#else
    screen->width = psx_get_display_width(screen->psx);
    screen->height = psx_get_display_height(screen->psx);
#endif
    
    if (screen->width >= 512) {
        screen->saved_scale = screen->scale;
        screen->scale = 1;
    } else {
        screen->scale = screen->saved_scale;
    }

    SDL_DestroyTexture(screen->texture);

    screen->texture = SDL_CreateTexture(
        screen->renderer,
        SDL_PIXELFORMAT_BGR555,
        SDL_TEXTUREACCESS_STREAMING,
        screen->width, screen->height
    );

    SDL_SetWindowSize(
        screen->window,
        screen->width * screen->scale,
        screen->height * screen->scale
    );
}