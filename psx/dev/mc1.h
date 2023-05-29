#ifndef MC1_H
#define MC1_H

#include <stdint.h>

#define PSX_MC1_BEGIN 0x1f801000
#define PSX_MC1_SIZE  0x24
#define PSX_MC1_END   0x1f801023

typedef struct {
    uint32_t io_base, io_size;

    uint32_t exp1_base;
    uint32_t exp2_base;
    uint32_t exp1_delay;
    uint32_t exp3_delay;
    uint32_t bios_delay;
    uint32_t spu_delay;
    uint32_t cdrom_delay;
    uint32_t exp2_delay;
    uint32_t com_delay;
} psx_mc1_t;

psx_mc1_t* psx_mc1_create();
void psx_mc1_init(psx_mc1_t*);
uint32_t psx_mc1_read32(psx_mc1_t*, uint32_t);
uint16_t psx_mc1_read16(psx_mc1_t*, uint32_t);
uint8_t psx_mc1_read8(psx_mc1_t*, uint32_t);
void psx_mc1_write32(psx_mc1_t*, uint32_t, uint32_t);
void psx_mc1_write16(psx_mc1_t*, uint32_t, uint16_t);
void psx_mc1_write8(psx_mc1_t*, uint32_t, uint8_t);
void psx_mc1_destroy(psx_mc1_t*);

#endif