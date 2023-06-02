#include <stdint.h>
#include <stdlib.h>

#include "gpu.h"
#include "../log.h"

typedef void (*psx_gpu_command_t)(psx_gpu_t*, uint32_t);

void gpu_cmd_invalid(psx_gpu_t* gpu, uint32_t cmd) {
    log_error("Invalid GPU command %02x (%08x)", cmd >> 24, cmd);

    exit(1);
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
            if (gpu->words_remaining) {
                gpu->fifo[gpu->fifo_index++] = value;

                log_error("ARG: %08x", value);

                gpu->words_remaining--;

                if (!gpu->words_remaining) {
                    gpu->fifo_index = 0;
                    gpu->read_done = 1;

                    gpu->pending_cmd(gpu);
                }
            } else {
                gpu->read_done = 0;

                uint8_t cmd = value >> 24;

                if ((cmd == 0x00) || ((cmd >= 0x04) && (cmd <= 0x1e)) ||
                    (cmd == 0xe0) || ((cmd >= 0xe7) && (cmd <= 0xef))) {
                    
                    // Execute instantly
                    return;
                }

                if (cmd == 0x02) { gpu->fifo[gpu->fifo_index++] = value; gpu->words_remaining = 2; gpu->pending_cmd = gpu_cmd_fillrect; }

                log_error("GP0(%02Xh) args=%06x", value >> 24, value & 0xffffff);
            }

            return;
        } break;

        // GP1
        case 0x04: {
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

void psx_gpu_destroy(psx_gpu_t* gpu) {
    free(gpu->vram);
    free(gpu);
}