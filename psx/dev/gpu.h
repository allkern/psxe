#ifndef GPU_H
#define GPU_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ic.h"

#define PSX_GPU_BEGIN 0x1f801810
#define PSX_GPU_SIZE  0x8
#define PSX_GPU_END   0x1f801814

#define PSX_GPU_FB_WIDTH 1024
#define PSX_GPU_FB_HEIGHT 512

#define PSX_GPU_VRAM_SIZE (0x100000)

enum {
    GPU_EVENT_DMODE,
    GPU_EVENT_VBLANK
};

enum {
    GPU_STATE_RECV_CMD,
    GPU_STATE_RECV_ARGS,
    GPU_STATE_RECV_DATA
};

struct psx_gpu_t;

typedef struct psx_gpu_t psx_gpu_t;

typedef void (*psx_gpu_cmd_t)(psx_gpu_t*);
typedef void (*psx_gpu_event_callback_t)(psx_gpu_t*);

typedef struct {
    int32_t x, y;
    uint32_t c;
    uint8_t tx, ty;
} vertex_t;

struct psx_gpu_t {
    uint32_t io_base, io_size;

    void* udata[4];

    uint16_t* vram;

    // State data
    uint32_t buf[16];
    uint32_t recv_data;
    int buf_index;
    int cmd_args_remaining;
    int cmd_data_remaining;

    // Command counters
    uint32_t color;
    uint32_t xpos, ypos;
    uint32_t xsiz, ysiz;
    uint32_t addr;
    uint32_t xcnt, ycnt;
    vertex_t v0, v1, v2, v3;
    uint32_t pal, texp;

    // GPU state
    uint32_t state;

    uint32_t display_mode;
    uint32_t gpuread;
    uint32_t gpustat;

    // Drawing area
    uint32_t draw_x1, draw_y1;
    uint32_t draw_x2, draw_y2;

    // Drawing offset
    uint32_t off_x, off_y;

    // Texture Window
    uint32_t texw_mx, texw_my;
    uint32_t texw_ox, texw_oy;

    // CLUT offset
    uint32_t clut_x, clut_y;

    // Texture page
    uint32_t texp_x, texp_y;
    uint32_t texp_d;

    // Display area
    uint32_t disp_x, disp_y;

    int cycles;

    psx_ic_t* ic;

    psx_gpu_event_callback_t event_cb_table[8];
};

psx_gpu_t* psx_gpu_create();
void psx_gpu_init(psx_gpu_t*, psx_ic_t*);
uint32_t psx_gpu_read32(psx_gpu_t*, uint32_t);
uint16_t psx_gpu_read16(psx_gpu_t*, uint32_t);
uint8_t psx_gpu_read8(psx_gpu_t*, uint32_t);
void psx_gpu_write32(psx_gpu_t*, uint32_t, uint32_t);
void psx_gpu_write16(psx_gpu_t*, uint32_t, uint16_t);
void psx_gpu_write8(psx_gpu_t*, uint32_t, uint8_t);
void psx_gpu_destroy(psx_gpu_t*);
void psx_gpu_set_udata(psx_gpu_t*, int, void*);
void psx_gpu_set_event_callback(psx_gpu_t*, int, psx_gpu_event_callback_t);
void psx_gpu_update(psx_gpu_t*);

#endif