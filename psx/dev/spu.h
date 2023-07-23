#ifndef SPU_H
#define SPU_H

#include <stdint.h>

#define PSX_SPU_BEGIN 0x1f801c00
#define PSX_SPU_SIZE  0x400
#define PSX_SPU_END   0x1f801fff

#define PSX_SPU_RAM_SIZE 0x80000

typedef struct {
    uint32_t io_base, io_size;

    uint8_t* ram;

    uint8_t r[0x400];

    uint32_t current_addr;

    // struct {
    //     uint16_t volumel;
    //     uint16_t volumer;
    //     uint16_t adsampr;
    //     uint16_t adsaddr;
    //     uint16_t envctl1;
    //     uint16_t envctl2;
    //     uint16_t envcvol;
    //     uint16_t adraddr;
    // } voice[24];

    // uint16_t mainvol[2];
    // uint16_t echovol[2];
    // uint32_t flags[3];
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