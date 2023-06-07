#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "gpu.h"
#include "../log.h"

uint16_t gpu_to_bgr555(uint32_t color) {
    return ((color & 0x0000f8) >> 3) |
           ((color & 0x00f800) >> 6) |
           ((color & 0xf80000) >> 9);
}

psx_gpu_t* psx_gpu_create() {
    return (psx_gpu_t*)malloc(sizeof(psx_gpu_t));
}

void psx_gpu_init(psx_gpu_t* gpu) {
    memset(gpu, 0, sizeof(psx_gpu_t));

    gpu->io_base = PSX_GPU_BEGIN;
    gpu->io_size = PSX_GPU_SIZE;

    gpu->vram = (uint16_t*)malloc(PSX_GPU_VRAM_SIZE);
    gpu->state = GPU_STATE_RECV_CMD;
}

uint32_t psx_gpu_read32(psx_gpu_t* gpu, uint32_t offset) {
    switch (offset) {
        case 0x00: return 0x00000000; // GPUREAD
        case 0x04: return 0x1c000000; // GPUSTAT
    }

    log_warn("Unhandled 32-bit GPU read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_gpu_read16(psx_gpu_t* gpu, uint32_t offset) {
    switch (offset) {
        case 0x00: return 0x00000000; // GPUREAD
        case 0x04: return 0x1c000000; // GPUSTAT
    }

    log_warn("Unhandled 16-bit GPU read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_gpu_read8(psx_gpu_t* gpu, uint32_t offset) {
    switch (offset) {
        case 0x00: return 0x00000000; // GPUREAD
        case 0x04: return 0x1c000000; // GPUSTAT
    }

    log_warn("Unhandled 8-bit GPU read at offset %08x", offset);

    return 0x0;
}

void gpu_render_flat_triangle(psx_gpu_t* gpu, vertex_t v0, vertex_t v1, vertex_t v2, uint32_t color) {
}

void gpu_cmd_a0(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                // Save static data
                gpu->xpos = gpu->buf[1] & 0x3ff;
                gpu->ypos = (gpu->buf[1] >> 16) & 0x1ff;
                gpu->xsiz = gpu->buf[2] & 0xffff;
                gpu->ysiz = gpu->buf[2] >> 16;
                gpu->xsiz = ((gpu->xsiz - 1) & 0x3ff) + 1;
                gpu->ysiz = ((gpu->ysiz - 1) & 0x1ff) + 1;
                gpu->addr = gpu->xpos + (gpu->ypos * 1024);
            }
        } break;

        case GPU_STATE_RECV_DATA: {
            uint32_t addr = gpu->addr;

            addr += gpu->xcnt;
            addr += gpu->ycnt * 1024;

            *(uint32_t*)(&gpu->vram[addr]) = gpu->recv_data;

            gpu->xcnt += 2;

            if ((gpu->xcnt == gpu->xsiz) && (gpu->ycnt == (gpu->ysiz - 1))) {
                gpu->xcnt = 0;
                gpu->ycnt = 0;

                gpu->state = GPU_STATE_RECV_CMD;
            } else if (gpu->xcnt == gpu->xsiz) {
                gpu->xcnt = 0;
                gpu->ycnt++;
            }
        } break;
    }
}

// Fill rectangle
void gpu_cmd_02(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->xpos = gpu->buf[1] & 0x3f0;
                gpu->ypos = (gpu->buf[1] >> 16) & 0x1ff;
                gpu->xsiz = gpu->buf[2] & 0xffff;
                gpu->ysiz = gpu->buf[2] >> 16;
                gpu->xsiz = ((gpu->xsiz & 0x3ff) + 0xf) & (~0xf);
                gpu->ysiz = gpu->ysiz & 0x1ff;
                gpu->color = gpu->buf[0] & 0xffffff;

                log_fatal("GPU fill pos=(%u, %u), siz=(%u, %u), color=%08x",
                    gpu->xpos, gpu->ypos,
                    gpu->xsiz, gpu->ysiz,
                    gpu->color
                );

                for (int y = gpu->ypos; y < gpu->ysiz; y++) {
                    for (int x = gpu->xpos; x < gpu->xsiz; x++) {
                        ((uint16_t*)gpu->vram)[x + (y * 512)] = 0x0000;
                    }
                }

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void psx_gpu_update_cmd(psx_gpu_t* gpu) {
    switch (gpu->buf[0] >> 24) {
        case 0x02: gpu_cmd_02(gpu); break;
        case 0xa0: gpu_cmd_a0(gpu); break;
    }
}

void psx_gpu_write32(psx_gpu_t* gpu, uint32_t offset, uint32_t value) {
    switch (offset) {
        // GP0
        case 0x00: {
            switch (gpu->state) {
                case GPU_STATE_RECV_CMD: {
                    gpu->buf_index = 0;
                    gpu->buf[gpu->buf_index++] = value;

                    psx_gpu_update_cmd(gpu);
                } break;

                case GPU_STATE_RECV_ARGS: {
                    gpu->buf[gpu->buf_index++] = value;
                    gpu->cmd_args_remaining--;

                    psx_gpu_update_cmd(gpu);
                } break;

                case GPU_STATE_RECV_DATA: {
                    gpu->recv_data = value;

                    psx_gpu_update_cmd(gpu);
                } break;
            }

            return;
        } break;

        // GP1
        case 0x04: {
            uint8_t cmd = value >> 24;

            switch (cmd) {
                case 0x08:
                    gpu->display_mode = value & 0xffffff;

                    if (gpu->dmode_event_cb)
                        gpu->dmode_event_cb(gpu);
                break;
            }

            log_error("GP1(%02Xh) args=%06x", value >> 24, value & 0xffffff);

            return;
        } break;
    }

    log_warn("Unhandled 32-bit GPU write at offset %08x (%08x)", offset, value);
}

void psx_gpu_write16(psx_gpu_t* gpu, uint32_t offset, uint16_t value) {
    log_warn("Unhandled 16-bit GPU write at offset %08x (%04x)", offset, value);
}

void psx_gpu_write8(psx_gpu_t* gpu, uint32_t offset, uint8_t value) {
    log_warn("Unhandled 8-bit GPU write at offset %08x (%02x)", offset, value);
}

void psx_gpu_set_dmode_event_callback(psx_gpu_t* gpu, psx_gpu_event_callback_t cb) {
    gpu->dmode_event_cb = cb;
}

void psx_gpu_set_udata(psx_gpu_t* gpu, int index, void* udata) {
    gpu->udata[index] = udata;
}

void psx_gpu_destroy(psx_gpu_t* gpu) {
    free(gpu->vram);
    free(gpu);
}