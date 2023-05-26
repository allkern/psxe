#include <stdint.h>
#include <stdlib.h>

#include "bus.h"
#include "log.h"

#define RANGE(v, s, e) ((v >= s) && (v <= e))

const uint32_t g_psx_bus_region_mask_table[] = {
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0x7fffffff, 0x1fffffff, 0xffffffff, 0xffffffff
};

psx_bus_t* psx_bus_create() {
    return (psx_bus_t*)malloc(sizeof(psx_bus_t));
}

void psx_bus_init(psx_bus_t* bus, psx_bios_t* bios, psx_ram_t* ram) {
    bus->bios = bios;
    bus->ram = ram;
}

void psx_bus_destroy(psx_bus_t* bus) {
    free(bus);
}

uint32_t psx_bus_read32(psx_bus_t* bus, uint32_t addr) {
    addr &= g_psx_bus_region_mask_table[addr >> 29];

    if (addr & 0x3) {
        log_warn("Unaligned 32-bit read from %08x", addr);
    }

    if (RANGE(addr, PSX_BIOS_BEGIN, PSX_BIOS_END))
        return psx_bios_read32(bus->bios, addr - PSX_BIOS_BEGIN);
    
    if (RANGE(addr, PSX_IO_RAM_SIZE_BEGIN, PSX_IO_RAM_SIZE_END))
        return bus->ram_size;
    
    if (RANGE(addr, PSX_RAM_BEGIN, PSX_RAM_END))
        return psx_ram_read32(bus->ram, addr - PSX_RAM_BEGIN);

    log_warn("Unhandled 32-bit read from %08x", addr);

    return 0xffffffff;
}

uint16_t psx_bus_read16(psx_bus_t* bus, uint32_t addr) {
    addr &= g_psx_bus_region_mask_table[addr >> 29];

    if (addr & 0x1) {
        log_warn("Unaligned 16-bit read from %08x", addr);
    }

    if (RANGE(addr, PSX_BIOS_BEGIN, PSX_BIOS_END))
        return psx_bios_read16(bus->bios, addr - PSX_BIOS_BEGIN);

    if (RANGE(addr, PSX_RAM_BEGIN, PSX_RAM_END))
        return psx_ram_read16(bus->ram, addr - PSX_RAM_BEGIN);

    log_warn("Unhandled 16-bit read from %08x", addr);

    return 0xffff;
}

uint8_t psx_bus_read8(psx_bus_t* bus, uint32_t addr) {
    addr &= g_psx_bus_region_mask_table[addr >> 29];

    if (RANGE(addr, PSX_BIOS_BEGIN, PSX_BIOS_END))
        return psx_bios_read8(bus->bios, addr - PSX_BIOS_BEGIN);

    if (RANGE(addr, PSX_RAM_BEGIN, PSX_RAM_END))
        return psx_ram_read8(bus->ram, addr - PSX_RAM_BEGIN);

    log_warn("Unhandled 8-bit read from %08x", addr);

    return 0xff;
}

void psx_bus_write32(psx_bus_t* bus, uint32_t addr, uint32_t value) {
    addr &= g_psx_bus_region_mask_table[addr >> 29];

    if (addr & 0x3) {
        log_warn("Unaligned 32-bit write to %08x (%08x)", addr, value);
    }

    if (RANGE(addr, PSX_IO_RAM_SIZE_BEGIN, PSX_IO_RAM_SIZE_END)) {
        bus->ram_size = value;

        return;
    }

    if (RANGE(addr, PSX_RAM_BEGIN, PSX_RAM_END)) {
        psx_ram_write32(bus->ram, addr - PSX_RAM_BEGIN, value);

        return;
    }

    log_warn("Unhandled 32-bit write to %08x (%08x)", addr, value);
}

void psx_bus_write16(psx_bus_t* bus, uint32_t addr, uint16_t value) {
    addr &= g_psx_bus_region_mask_table[addr >> 29];

    if (addr & 0x1) {
        log_warn("Unaligned 16-bit write to %08x (%08x)", addr, value);
    }

    if (RANGE(addr, PSX_RAM_BEGIN, PSX_RAM_END)) {
        psx_ram_write16(bus->ram, addr - PSX_RAM_BEGIN, value);

        return;
    }

    log_warn("Unhandled 32-bit write to %08x (%04x)", addr, value);
}

void psx_bus_write8(psx_bus_t* bus, uint32_t addr, uint8_t value) {
    addr &= g_psx_bus_region_mask_table[addr >> 29];

    if (RANGE(addr, PSX_RAM_BEGIN, PSX_RAM_END)) {
        psx_ram_write8(bus->ram, addr - PSX_RAM_BEGIN, value);

        return;
    }

    log_warn("Unhandled 32-bit write to %08x (%02x)", addr, value);
}
