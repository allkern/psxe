#ifndef GPU_H
#define GPU_H

#include <stdint.h>

#define PSX_GPU_BEGIN 0x1f801810
#define PSX_GPU_SIZE  0x8
#define PSX_GPU_END   0x1f801814

#define PSX_GPU_VRAM_SIZE 0x100000

struct psx_gpu_t;

typedef struct psx_gpu_t psx_gpu_t;

typedef void (*psx_gpu_cmd_t)(psx_gpu_t*);

struct psx_gpu_t {
    uint32_t io_base, io_size;

    uint8_t* vram;

    uint32_t fifo[16];
    int fifo_index;

    psx_gpu_cmd_t pending_cmd;

    int words_remaining;
    int read_done;
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

#endif