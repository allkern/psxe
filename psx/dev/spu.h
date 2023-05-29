#ifndef SPU_H
#define SPU_H

#include <stdint.h>

#define PSX_SPU_BEGIN 0x1f801c00
#define PSX_SPU_SIZE  0x400
#define PSX_SPU_END   0x1f801fff

typedef struct {
    uint32_t io_base, io_size;
} psx_spu_t;

psx_spu_t* psx_spu_create();
void psx_spu_init(psx_spu_t*);
uint32_t psx_spu_read32(psx_spu_t*, uint32_t);
uint16_t psx_spu_read16(psx_spu_t*, uint32_t);
uint8_t psx_spu_read8(psx_spu_t*, uint32_t);
void psx_spu_write32(psx_spu_t*, uint32_t, uint32_t);
void psx_spu_write16(psx_spu_t*, uint32_t, uint16_t);
void psx_spu_write8(psx_spu_t*, uint32_t, uint8_t);
void psx_spu_destroy(psx_spu_t*);

#endif