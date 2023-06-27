#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cdrom.h"
#include "../log.h"

#define STAT_INDEX_MASK   0x3
#define STAT_ADPBUSY_MASK 0x4
#define STAT_PRMEMPT_MASK 0x8
#define STAT_PRMWRDY_MASK 0x10
#define STAT_RSLRRDY_MASK 0x20
#define STAT_DRQSTS_MASK  0x40
#define STAT_BUSYSTS_MASK 0x80
#define STAT_INDEX   (cdrom->status & STAT_INDEX_MASK)
#define STAT_ADPBUSY (cdrom->status & STAT_ADPBUSY_MASK)
#define STAT_PRMEMPT (cdrom->status & STAT_PRMEMPT_MASK)
#define STAT_PRMWRDY (cdrom->status & STAT_PRMWRDY_MASK)
#define STAT_RSLRRDY (cdrom->status & STAT_RSLRRDY_MASK)
#define STAT_DRQSTS  (cdrom->status & STAT_DRQSTS_MASK)
#define STAT_BUSYSTS (cdrom->status & STAT_BUSYSTS_MASK)
#define SET_BITS(reg, mask, v) { cdrom->reg &= ~mask; cdrom->reg |= v & mask; }
#define IFR_INT  0x07
#define IFR_INT1 0x01
#define IFR_INT2 0x02
#define IFR_INT3 0x03
#define IFR_INT4 0x04
#define IFR_INT5 0x05
#define IFR_INT6 0x06
#define IFR_INT7 0x07

typedef uint8_t (*psx_cdrom_read_function_t)(psx_cdrom_t*);
typedef void (*psx_cdrom_write_function_t)(psx_cdrom_t*, uint8_t);

psx_cdrom_read_function_t g_psx_cdrom_read_table[] = {
    cdrom_read_status, cdrom_read_rfifo, cdrom_read_dfifo, cdrom_read_ier,
    cdrom_read_status, cdrom_read_rfifo, cdrom_read_dfifo, cdrom_read_ifr,
    cdrom_read_status, cdrom_read_rfifo, cdrom_read_dfifo, cdrom_read_ier,
    cdrom_read_status, cdrom_read_rfifo, cdrom_read_dfifo, cdrom_read_ifr
};

psx_cdrom_write_function_t g_psx_cdrom_write_table[] = {
    cdrom_write_status, cdrom_write_cmd     , cdrom_write_pfifo   , cdrom_write_req     ,
    cdrom_write_status, cdrom_write_smdout  , cdrom_write_ier     , cdrom_write_ifr     ,
    cdrom_write_status, cdrom_write_sminfo  , cdrom_write_lcdlspuv, cdrom_write_lcdrspuv,
    cdrom_write_status, cdrom_write_rcdrspuv, cdrom_write_rcdlspuv, cdrom_write_volume
};

uint8_t cdrom_read_status(psx_cdrom_t* cdrom) {
    log_fatal("    Status read %02x", cdrom->status);

    return cdrom->status;
}

uint8_t cdrom_read_rfifo(psx_cdrom_t* cdrom) {
    log_fatal("    RFIFO read (%02x)", cdrom->rfifo[cdrom->rfifo_index - 1]);
    
    if (!cdrom->rfifo_index) {
        uint8_t data = cdrom->rfifo[cdrom->rfifo_index];
        
        SET_BITS(status, STAT_RSLRRDY_MASK, 0);

        return data;
    } else {
        return cdrom->rfifo[--cdrom->rfifo_index];
    }
}

uint8_t cdrom_read_dfifo(psx_cdrom_t* cdrom) {
    log_fatal("Data FIFO unimplemented");

    return 0x00;
}

uint8_t cdrom_read_ier(psx_cdrom_t* cdrom) {
    log_fatal("    IER read %02x", cdrom->ier);

    return cdrom->ier;
}

uint8_t cdrom_read_ifr(psx_cdrom_t* cdrom) {
    log_fatal("    IFR read %02x", cdrom->ifr);

    return cdrom->ifr;
}

void cdrom_write_status(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("    Status write %02x", value);

    SET_BITS(status, STAT_INDEX_MASK, value);
}

void cdrom_write_cmd(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("    Command %02x (pfifo=%02x, %02x, %02x, %02x)",
        value,
        cdrom->pfifo[0],
        cdrom->pfifo[1],
        cdrom->pfifo[2],
        cdrom->pfifo[3]
    );

    switch (value) {
        // Getstat
        case 0x01: {
            cdrom->rfifo[cdrom->rfifo_index++] = 0x10;

            SET_BITS(status, STAT_RSLRRDY_MASK, STAT_RSLRRDY_MASK);
            SET_BITS(ifr, IFR_INT, IFR_INT3);

            cdrom->irq_delay = 0xc4e0;
        } break;

        // test
        case 0x19: {
            switch (cdrom->pfifo[0]) {
                case 0x20: {
                    cdrom->rfifo[cdrom->rfifo_index++] = 0x01;
                    cdrom->rfifo[cdrom->rfifo_index++] = 0x95;
                    cdrom->rfifo[cdrom->rfifo_index++] = 0x13;
                    cdrom->rfifo[cdrom->rfifo_index++] = 0x03;

                    SET_BITS(status, STAT_RSLRRDY_MASK, STAT_RSLRRDY_MASK);
                    SET_BITS(ifr, IFR_INT, IFR_INT3);

                    cdrom->irq_delay = 0xc4e0;
                } break;
            }
        } break;
    }
}

void cdrom_write_pfifo(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("    PFIFO write %02x", value);

    cdrom->pfifo[(cdrom->pfifo_index++) & 0xf] = value;

    SET_BITS(status, STAT_PRMWRDY_MASK, (cdrom->pfifo_index & 0x10) ? 0xff : 0);

    cdrom->pfifo_index &= 0x1f;
}

void cdrom_write_req(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Request register unimplemented");
}

void cdrom_write_smdout(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Sound map data out unimplemented");
}

void cdrom_write_ier(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("    IER write %02x", value);

    cdrom->ier = value;
}

void cdrom_write_ifr(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("    IFR write %02x", value);

    cdrom->ifr &= ~(value & 0x7);

    if (value & 0x40) {
        cdrom->pfifo_index = 0;

        SET_BITS(
            status,
            (STAT_PRMEMPT_MASK | STAT_PRMWRDY_MASK),
            (STAT_PRMEMPT_MASK | STAT_PRMWRDY_MASK)
        );
    }
}

void cdrom_write_sminfo(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Sound map coding info unimplemented");
}

void cdrom_write_lcdlspuv(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Volume registers unimplemented");
}

void cdrom_write_rcdrspuv(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Volume registers unimplemented");
}

void cdrom_write_rcdlspuv(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Volume registers unimplemented");
}

void cdrom_write_lcdrspuv(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Volume registers unimplemented");
}

void cdrom_write_volume(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Volume registers unimplemented");
}


psx_cdrom_t* psx_cdrom_create() {
    return (psx_cdrom_t*)malloc(sizeof(psx_cdrom_t));
}

void psx_cdrom_init(psx_cdrom_t* cdrom, psx_ic_t* ic) {
    memset(cdrom, 0, sizeof(psx_cdrom_t));

    cdrom->io_base = PSX_CDROM_BEGIN;
    cdrom->io_size = PSX_CDROM_SIZE;

    cdrom->ic = ic;
    cdrom->status = STAT_PRMEMPT_MASK | STAT_PRMWRDY_MASK;
}

uint32_t psx_cdrom_read32(psx_cdrom_t* cdrom, uint32_t offset) {
    log_fatal("Unhandled 32-bit CDROM read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_cdrom_read16(psx_cdrom_t* cdrom, uint32_t offset) {
    log_fatal("Unhandled 16-bit CDROM read at offset %08x", offset);

    return 0x0;
}

uint8_t psx_cdrom_read8(psx_cdrom_t* cdrom, uint32_t offset) {
    return g_psx_cdrom_read_table[(STAT_INDEX << 2) | offset](cdrom);
}

void psx_cdrom_write32(psx_cdrom_t* cdrom, uint32_t offset, uint32_t value) {
    log_fatal("Unhandled 32-bit CDROM write at offset %08x (%08x)", offset, value);
}

void psx_cdrom_write16(psx_cdrom_t* cdrom, uint32_t offset, uint16_t value) {
    log_fatal("Unhandled 16-bit CDROM write at offset %08x (%04x)", offset, value);
}

void psx_cdrom_write8(psx_cdrom_t* cdrom, uint32_t offset, uint8_t value) {
    g_psx_cdrom_write_table[(STAT_INDEX << 2) | offset](cdrom, value);
}

void psx_cdrom_update(psx_cdrom_t* cdrom) {
    if (cdrom->irq_delay) {
        cdrom->irq_delay -= 2;

        if (!cdrom->irq_delay)
            psx_ic_irq(cdrom->ic, IC_CDROM);
    }
}

void psx_cdrom_destroy(psx_cdrom_t* cdrom) {
    free(cdrom);
}