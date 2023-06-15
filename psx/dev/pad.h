#ifndef PAD_H
#define PAD_H

#include <stdint.h>

#define PSX_PAD_BEGIN 0x1f801040
#define PSX_PAD_SIZE  0x10
#define PSX_PAD_END   0x1f80104f

typedef struct {
    uint32_t io_base, io_size;

    uint16_t mode, ctrl, baud;
} psx_pad_t;

psx_pad_t* psx_pad_create();
void psx_pad_init(psx_pad_t*);
uint32_t psx_pad_read32(psx_pad_t*, uint32_t);
uint16_t psx_pad_read16(psx_pad_t*, uint32_t);
uint8_t psx_pad_read8(psx_pad_t*, uint32_t);
void psx_pad_write32(psx_pad_t*, uint32_t, uint32_t);
void psx_pad_write16(psx_pad_t*, uint32_t, uint16_t);
void psx_pad_write8(psx_pad_t*, uint32_t, uint8_t);
void psx_pad_destroy(psx_pad_t*);

#endif