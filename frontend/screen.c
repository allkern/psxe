#include "screen.h"

uint16_t screen_get_button(SDL_Keycode k) {
    if (k == SDLK_x     ) return PSXI_SW_SDA_CROSS;
    if (k == SDLK_a     ) return PSXI_SW_SDA_SQUARE;
    if (k == SDLK_w     ) return PSXI_SW_SDA_TRIANGLE;
    if (k == SDLK_d     ) return PSXI_SW_SDA_CIRCLE;
    if (k == SDLK_RETURN) return PSXI_SW_SDA_START;
    if (k == SDLK_q     ) return PSXI_SW_SDA_SELECT;
    if (k == SDLK_UP    ) return PSXI_SW_SDA_PAD_UP;
    if (k == SDLK_DOWN  ) return PSXI_SW_SDA_PAD_DOWN;
    if (k == SDLK_LEFT  ) return PSXI_SW_SDA_PAD_LEFT;
    if (k == SDLK_RIGHT ) return PSXI_SW_SDA_PAD_RIGHT;

    return 0;
}

psxe_screen_t* psxe_screen_create() {
    return (psxe_screen_t*)malloc(sizeof(psxe_screen_t));
}

void psxe_screen_init(psxe_screen_t* screen, psx_t* psx) {
    memset(screen, 0, sizeof(psxe_screen_t));

    if (screen->debug_mode) {
        screen->width = PSX_GPU_FB_WIDTH;
        screen->height = PSX_GPU_FB_HEIGHT;
    } else {
        screen->width = 320;
        screen->height = 240;
    }

    screen->scale = 1;
    screen->open = 1;
    screen->format = SDL_PIXELFORMAT_BGR555;
    screen->psx = psx;
    screen->pad = psx_get_pad(psx);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_SetHint("SDL_HINT_RENDER_SCALE_QUALITY", "0");
}

void psxe_screen_reload(psxe_screen_t* screen) {
    SDL_SetHint("SDL_HINT_RENDER_SCALE_QUALITY", "0");

    if (screen->texture) SDL_DestroyTexture(screen->texture);
    if (screen->renderer) SDL_DestroyRenderer(screen->renderer);
    if (screen->window) SDL_DestroyWindow(screen->window);

    screen->window = SDL_CreateWindow(
        "psxe " STR(REP_VERSION) "-" STR(REP_COMMIT_HASH),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screen->width * screen->scale,
        screen->height * screen->scale,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
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

    // Check for retina displays
    int width = 0, height = 0;

    SDL_GetRendererOutputSize(screen->renderer, &width, &height);

    if (width != (screen->width * screen->scale)) {
        float width_scale = (float)width / (float)(screen->width * screen->scale);
        float height_scale = (float)height / (float)(screen->height * screen->scale);

        SDL_RenderSetScale(screen->renderer, width_scale, height_scale);
    }

    screen->open = 1;
}

int psxe_screen_is_open(psxe_screen_t* screen) {
    return screen->open;
}

void psxe_screen_toggle_debug_mode(psxe_screen_t* screen) {
    screen->debug_mode = !screen->debug_mode;

    psxe_gpu_dmode_event_cb(screen->psx->gpu);
}

void psxe_screen_update(psxe_screen_t* screen) {
    void* display_buf = screen->debug_mode ?
        psx_get_vram(screen->psx) : psx_get_display_buffer(screen->psx);

    SDL_UpdateTexture(screen->texture, NULL, display_buf, PSX_GPU_FB_STRIDE);
    SDL_RenderCopy(screen->renderer, screen->texture, NULL, NULL);
    SDL_RenderPresent(screen->renderer);

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                screen->open = 0;
            } break;

            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                    case SDLK_F1: {
                        psxe_screen_toggle_debug_mode(screen);

                        return;
                    } break;
                }

                uint16_t mask = screen_get_button(event.key.keysym.sym);

                psx_pad_button_press(screen->pad, 0, mask);
            } break;

            case SDL_KEYUP: {
                uint16_t mask = screen_get_button(event.key.keysym.sym);

                psx_pad_button_release(screen->pad, 0, mask);
            } break;
        }
    }
}

void psxe_screen_set_scale(psxe_screen_t* screen, unsigned int scale) {
    if (screen->debug_mode) {
        screen->scale = 1;
    } else {
        screen->scale = scale;
        screen->saved_scale = scale;
    }
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

    screen->format = psx_get_display_format(screen->psx) ?
        SDL_PIXELFORMAT_BGR555 : SDL_PIXELFORMAT_RGB888;

    if (screen->debug_mode) {
        screen->width = PSX_GPU_FB_WIDTH;
        screen->height = PSX_GPU_FB_HEIGHT;
    } else {
        screen->width = psx_get_display_width(screen->psx);
        screen->height = psx_get_display_height(screen->psx);
    }

    if (screen->width > 512) {
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

void psxe_gpu_vblank_event_cb(psx_gpu_t* gpu) {
    psxe_screen_t* screen = gpu->udata[0];

    psxe_screen_update(screen);
}