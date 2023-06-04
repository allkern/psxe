#ifndef SCREEN_H
#define SCREEN_H

#include "psx/dev/gpu.h"

#include <string.h>

#include "SDL.h"

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    unsigned int width, height, scale;
    unsigned int format;
    unsigned int mode;
    int open;

    uint32_t* buf;
} psxe_screen_t;

psxe_screen_t* psxe_screen_create();
void psxe_screen_init(psxe_screen_t*, psx_gpu_t*);
void psxe_screen_reload(psxe_screen_t*);
int psxe_screen_is_open(psxe_screen_t*);
void psxe_screen_update(psxe_screen_t*);
void psxe_screen_destroy(psxe_screen_t*);
void psxe_screen_set_scale(psxe_screen_t*, unsigned int);

// GPU event handlers
void psxe_gpu_dmode_event_cb(psx_gpu_t*);

#endif