#ifndef GPU_H
#define GPU_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PSX_GPU_BEGIN 0x1f801810
#define PSX_GPU_SIZE  0x8
#define PSX_GPU_END   0x1f801814

#define PSX_GPU_FB_WIDTH 1024
#define PSX_GPU_FB_HEIGHT 512

#define PSX_GPU_VRAM_SIZE 0x100000

enum {
    GPU_STATE_RECV_CMD,
    GPU_STATE_RECV_ARGS,
    GPU_STATE_RECV_DATA
};

struct psx_gpu_t;

typedef struct psx_gpu_t psx_gpu_t;

typedef void (*psx_gpu_cmd_t)(psx_gpu_t*);
typedef void (*psx_gpu_event_callback_t)(psx_gpu_t*);

struct psx_gpu_t {
    uint32_t io_base, io_size;

    void* udata[4];

    uint8_t* vram;

    uint32_t buf[16];
    uint32_t recv_data;
    int buf_index;
    int cmd_args_remaining;
    int cmd_data_remaining;

    int words_remaining;
    int read_done;

    uint32_t color;
    uint32_t xpos, ypos;
    uint32_t xsiz, ysiz;
    uint32_t addr;

    int cmd_a0_receiving_pos;
    int cmd_a0_receiving_size;
    int cmd_a0_receiving_data;
    uint32_t cmd_a0_xpos;
    uint32_t cmd_a0_ypos;
    uint32_t cmd_a0_xsiz;
    uint32_t cmd_a0_ysiz;
    uint32_t xcnt, ycnt;

    uint32_t state;

    uint32_t display_mode;
    uint32_t gpuread;

    psx_gpu_event_callback_t dmode_event_cb;
};

psx_gpu_t* psx_gpu_create();
void psx_gpu_init(psx_gpu_t*);
uint32_t psx_gpu_read32(psx_gpu_t*, uint32_t);
uint16_t psx_gpu_read16(psx_gpu_t*, uint32_t);
uint8_t psx_gpu_read8(psx_gpu_t*, uint32_t);
void psx_gpu_write32(psx_gpu_t*, uint32_t, uint32_t);
void psx_gpu_write16(psx_gpu_t*, uint32_t, uint16_t);
void psx_gpu_write8(psx_gpu_t*, uint32_t, uint8_t);
void psx_gpu_destroy(psx_gpu_t*);
void psx_gpu_set_udata(psx_gpu_t*, int, void*);
void psx_gpu_set_dmode_event_callback(psx_gpu_t*, psx_gpu_event_callback_t);

#endif