#ifndef MDEC_H
#define MDEC_H

#include <stdint.h>

#include "../log.h"

#define PSX_MDEC_SIZE    0x8
#define PSX_MDEC_BEGIN   0x1f801820
#define PSX_MDEC_END     0x1f801827

enum {
    MDEC_RECV_CMD,
    MDEC_RECV_BLOCK,
    MDEC_RECV_QUANT,
    MDEC_RECV_QUANT_COLOR,
    MDEC_RECV_SCALE
};

typedef struct {
    uint32_t io_base, io_size;

    uint32_t cmd;

    int state;
    int data_remaining;
    int index;

    uint32_t status;
} psx_mdec_t;

psx_mdec_t* psx_mdec_create();
void psx_mdec_init(psx_mdec_t*);
uint32_t psx_mdec_read32(psx_mdec_t*, uint32_t);
uint16_t psx_mdec_read16(psx_mdec_t*, uint32_t);
uint8_t psx_mdec_read8(psx_mdec_t*, uint32_t);
void psx_mdec_write32(psx_mdec_t*, uint32_t, uint32_t);
void psx_mdec_write16(psx_mdec_t*, uint32_t, uint16_t);
void psx_mdec_write8(psx_mdec_t*, uint32_t, uint8_t);
void psx_mdec_destroy(psx_mdec_t*);

typedef void (*mdec_fn_t)(psx_mdec_t*);

#endif