#ifndef SPU_H
#define SPU_H

#include <stdint.h>

#define PSX_SPU_BEGIN 0x1f801c00
#define PSX_SPU_SIZE  0x400
#define PSX_SPU_END   0x1f801fff

#define SPU_RAM_SIZE 0x80000

/*
    1F801D88h - Voice 0..23 Key ON (Start Attack/Decay/Sustain) (KON) (W)
    1F801D8Ch - Voice 0..23 Key OFF (Start Release) (KOFF) (W)
    1F801D9Ch - Voice 0..23 ON/OFF (status) (ENDX) (R)
    1F801DA6h - Sound RAM Data Transfer Address
    1F801DA8h - Sound RAM Data Transfer Fifo
    1F801DACh - Sound RAM Data Transfer Control (should be 0004h)
*/

#define SPUR_KON     0x188
#define SPUR_KOFF    0x18c
#define SPUR_ENDX    0x19c
#define SPUR_TADDR   0x1a6
#define SPUR_TFIFO   0x1a8
#define SPUR_SPUCNT  0x1aa
#define SPUR_TCTRL   0x1ac
#define SPUR_SPUSTAT 0x1ae

typedef struct __attribute__((__packed__)) {
    uint32_t io_base, io_size;

    uint8_t* ram;

    struct {
        uint16_t volumel;
        uint16_t volumer;
        uint16_t adsampr;
        uint16_t adsaddr;
        uint16_t envctl1;
        uint16_t envctl2;
        uint16_t envcvol;
        uint16_t adraddr;
    } voice[24];

    uint16_t mainlvol;
    uint16_t mainrvol;
    uint16_t revblvol;
    uint16_t revbrvol;
    uint32_t kon;
    uint32_t koff;
    uint32_t pmon;
    uint32_t non;
    uint32_t eon;
    uint32_t endx;
    uint16_t unk_da0;
    uint16_t revbaddr;
    uint16_t irq9addr;
    uint16_t ramdta;
    uint16_t ramdtf;
    uint16_t spucnt;
    uint16_t ramdtc;
    uint16_t spustat;
    uint32_t cdaivol;
    uint32_t extivol;
    uint32_t currvol;
    uint32_t unk_dbc;
    uint16_t dapf1;
    uint16_t dapf2;
    uint16_t viir;
    uint16_t vcomb1;
    uint16_t vcomb2;
    uint16_t vcomb3;
    uint16_t vcomb4;
    uint16_t vwall;
    uint16_t vapf1;
    uint16_t vapf2;
    uint16_t mlsame;
    uint16_t mrsame;
    uint16_t mlcomb1;
    uint16_t mrcomb1;
    uint16_t mlcomb2;
    uint16_t mrcomb2;
    uint16_t dlsame;
    uint16_t drsame;
    uint16_t mldiff;
    uint16_t mrdiff;
    uint16_t mlcomb3;
    uint16_t mrcomb3;
    uint16_t mlcomb4;
    uint16_t mrcomb4;
    uint16_t dldiff;
    uint16_t drdiff;
    uint16_t mlapf1;
    uint16_t mrapf1;
    uint16_t mlapf2;
    uint16_t mrapf2;
    uint16_t vlin;
    uint16_t vrin;

    // Internal registers unimplemented

    uint32_t taddr;
    uint16_t tfifo[32];
    uint16_t tfifo_index;

    struct {
        uint32_t counter;
        uint32_t current_addr;
        uint32_t repeat_addr;
        int16_t s[4];
        int block_flags;
    } data[24];
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
int16_t psx_spu_get_sample(psx_spu_t*);

#endif