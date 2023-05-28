#ifndef IC_H
#define IC_H

#include <stdint.h>

#define PSX_IC_BEGIN 0x1f801070
#define PSX_IC_SIZE  0x8
#define PSX_IC_END   0x1F801077

/*
  1F801070h 2    I_STAT - Interrupt status register
  1F801074h 2    I_MASK - Interrupt mask register
*/

typedef struct {
    uint32_t io_base, io_size;

    uint16_t i_stat;
    uint16_t i_mask;
} psx_ic_t;

psx_ic_t* psx_ic_create();
void psx_ic_init(psx_ic_t*);
uint32_t psx_ic_read32(psx_ic_t*, uint32_t);
uint16_t psx_ic_read16(psx_ic_t*, uint32_t);
uint8_t psx_ic_read8(psx_ic_t*, uint32_t);
void psx_ic_write32(psx_ic_t*, uint32_t, uint32_t);
void psx_ic_write16(psx_ic_t*, uint32_t, uint16_t);
void psx_ic_write8(psx_ic_t*, uint32_t, uint8_t);
void psx_ic_destroy(psx_ic_t*);

#endif