#ifndef CDROM_H
#define CDROM_H

#include <stdint.h>

#include "ic.h"

#define PSX_CDROM_BEGIN 0x1f801800
#define PSX_CDROM_SIZE  0x4
#define PSX_CDROM_END   0x1f801803

typedef struct {
    uint32_t io_base, io_size;

    psx_ic_t* ic;

    uint8_t status;
    uint8_t ifr;
    uint8_t ier;

    uint8_t pfifo[16];
    uint8_t rfifo[16];
    int pfifo_index;
    int rfifo_index;

    int irq_delay;
} psx_cdrom_t;

psx_cdrom_t* psx_cdrom_create();
void psx_cdrom_init(psx_cdrom_t*, psx_ic_t*);
uint32_t psx_cdrom_read32(psx_cdrom_t*, uint32_t);
uint16_t psx_cdrom_read16(psx_cdrom_t*, uint32_t);
uint8_t psx_cdrom_read8(psx_cdrom_t*, uint32_t);
void psx_cdrom_write32(psx_cdrom_t*, uint32_t, uint32_t);
void psx_cdrom_write16(psx_cdrom_t*, uint32_t, uint16_t);
void psx_cdrom_write8(psx_cdrom_t*, uint32_t, uint8_t);
void psx_cdrom_update(psx_cdrom_t*);
void psx_cdrom_destroy(psx_cdrom_t*);

#endif