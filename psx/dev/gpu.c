#include <stdint.h>
#include <stdlib.h>

#include "gpu.h"
#include "../log.h"

typedef void (*psx_gpu_command_t)(psx_gpu_t*, uint32_t);

void gpu_cmd_set_drawing_area_1(psx_gpu_t* gpu, uint32_t cmd) {

}

void gpu_cmd_invalid(psx_gpu_t* gpu, uint32_t cmd) {
    log_error("Invalid GPU command %02x (%08x)", cmd >> 24, cmd);

    exit(1);
}

void gpu_cmd_nop(psx_gpu_t* gpu, uint32_t cmd) {
    log_error("GP0(%02xh) - NOP", cmd >> 24);
    // Do nothing
}

psx_gpu_command_t g_psx_gpu_command_table[] = {
    gpu_cmd_nop,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid,
    gpu_cmd_invalid
};

psx_gpu_t* psx_gpu_create() {
    return (psx_gpu_t*)malloc(sizeof(psx_gpu_t));
}

void psx_gpu_init(psx_gpu_t* gpu) {
    gpu->io_base = PSX_GPU_BEGIN;
    gpu->io_size = PSX_GPU_SIZE;
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

void psx_gpu_write32(psx_gpu_t* gpu, uint32_t offset, uint32_t value) {
    switch (offset) {
        // GP0
        case 0x00: {
            log_error("GP0(%02Xh) args=%06x", value >> 24, value & 0xffffff);
            //g_psx_gpu_command_table[value >> 24](gpu, value);
        } break;

        // GP1
        case 0x04: {

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
    free(gpu);
}