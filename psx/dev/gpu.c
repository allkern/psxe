#include <stdint.h>
#include <stdlib.h>

#include "gpu.h"
#include "../log.h"

typedef void (*psx_gpu_command_t)(psx_gpu_t*, uint32_t);

void gpu_cmd_invalid(psx_gpu_t* gpu, uint32_t cmd) {
    log_error("Invalid GPU command %02x (%08x)", cmd >> 24, cmd);

    //exit(1);
}

void gpu_cmd_nop(psx_gpu_t* gpu, uint32_t cmd) {
    log_error("GP0(%02xh) - NOP", cmd >> 24);
    // Do nothing
}

psx_gpu_t* psx_gpu_create() {
    return (psx_gpu_t*)malloc(sizeof(psx_gpu_t));
}

void psx_gpu_init(psx_gpu_t* gpu) {
    gpu->io_base = PSX_GPU_BEGIN;
    gpu->io_size = PSX_GPU_SIZE;

    gpu->cmd_a0_receiving_data = 0;
    gpu->cmd_a0_receiving_size = 0;
    gpu->cmd_a0_receiving_pos = 0;

    gpu->vram = (uint8_t*)malloc(PSX_GPU_VRAM_SIZE);
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

// GP0(02h)
void gpu_cmd_fillrect(psx_gpu_t* gpu) {
    uint32_t color = gpu->fifo[0] & 0xffffff;
    uint32_t xpos = gpu->fifo[1] & 0xffff;
    uint32_t ypos = gpu->fifo[1] >> 16;
    uint32_t xsiz = gpu->fifo[2] & 0xffff;
    uint32_t ysiz = gpu->fifo[2] >> 16;

    log_error("GP0(02h) FillRect: [0]=%08x, [1]=%08x, [2]=%08x, color=%06x, xpos=%u (%u), ypos=%u, xsiz=%u (%u), ysiz=%u", gpu->fifo[0], gpu->fifo[1], gpu->fifo[2], color & 0xffffff, xpos, xpos << 4, ypos, xsiz, xsiz << 4, ysiz);
}

void psx_gpu_write32(psx_gpu_t* gpu, uint32_t offset, uint32_t value) {
    switch (offset) {
        // GP0
        case 0x00: {
            if (gpu->cmd_a0_receiving_pos) {
                gpu->cmd_a0_xpos = (value & 0xffff) >> 1;
                gpu->cmd_a0_ypos = value >> 16;

                gpu->cmd_a0_xpos = (gpu->cmd_a0_xpos & 0x3ff);
                gpu->cmd_a0_ypos = (gpu->cmd_a0_ypos & 0x1ff);

                gpu->cmd_a0_receiving_pos = 0;
                gpu->cmd_a0_receiving_size = 1;

                return;
            }

            if (gpu->cmd_a0_receiving_size) {
                gpu->cmd_a0_xsiz = (value & 0xffff) >> 1;
                gpu->cmd_a0_ysiz = value >> 16;

                gpu->cmd_a0_xsiz = ((gpu->cmd_a0_xsiz-1) & 0x3ff)+1;
                gpu->cmd_a0_ysiz = ((gpu->cmd_a0_ysiz-1) & 0x1ff)+1;

                gpu->xcnt = 0;
                gpu->ycnt = 0;

                gpu->cmd_a0_receiving_size = 0;
                gpu->cmd_a0_receiving_data = 1;

                return;
            }

            if (gpu->cmd_a0_receiving_data) {
                uint32_t addr = (gpu->cmd_a0_xpos << 2) + (gpu->cmd_a0_ypos * 1024);

                addr += gpu->xcnt * 4;
                addr += gpu->ycnt * 1024;

                if (addr < (1024 * 512))
                    *(uint32_t*)(gpu->vram + addr) = value;

                gpu->xcnt++;

                if (gpu->xcnt == gpu->cmd_a0_xsiz) {
                    gpu->xcnt = 0;
                    gpu->ycnt++;
                }

                if ((gpu->xcnt == gpu->cmd_a0_xsiz) && (gpu->ycnt == gpu->cmd_a0_ysiz)) {
                    gpu->xcnt = 0;
                    gpu->ycnt = 0;

                    gpu->cmd_a0_receiving_data = 0;
                }
            }

            uint8_t cmd = value >> 24;

            //log_fatal("GP0(%02Xh) args=%06x", value >> 24, value & 0xffffff);

            if (cmd == 0xa0) {
                gpu->cmd_a0_receiving_pos = 1;
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