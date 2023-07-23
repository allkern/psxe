#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cdrom.h"
#include "../log.h"

#define RESP_PUSH(data) \
    cdrom->rfifo[cdrom->rfifo_index++] = data; \
    SET_BITS(status, STAT_RSLRRDY_MASK, STAT_RSLRRDY_MASK);

#define PFIFO_POP (cdrom->pfifo[--cdrom->pfifo_index])

#define SEND_INT1(delay) cdrom->int_number = IFR_INT1; cdrom->irq_delay = delay;
#define SEND_INT2(delay) cdrom->int_number = IFR_INT2; cdrom->irq_delay = delay;
#define SEND_INT3(delay) cdrom->int_number = IFR_INT3; cdrom->irq_delay = delay;
#define SEND_INT4(delay) cdrom->int_number = IFR_INT4; cdrom->irq_delay = delay;
#define SEND_INT5(delay) cdrom->int_number = IFR_INT5; cdrom->irq_delay = delay;
#define SEND_INT6(delay) cdrom->int_number = IFR_INT6; cdrom->irq_delay = delay;
#define SEND_INT7(delay) cdrom->int_number = IFR_INT7; cdrom->irq_delay = delay;

uint8_t cdrom_btoi(uint8_t b) {
    return ((b >> 4) * 10) + (b & 0xf);
}

static const uint8_t g_psx_cdrom_btoi_table[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
    0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43,
    0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
    0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d,
    0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61,
    0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
    0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91,
    0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93,
    0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d,
    0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
};

void cdrom_cmd_error(psx_cdrom_t* cdrom) {
    SET_BITS(ifr, IFR_INT, IFR_INT5);
    RESP_PUSH(cdrom->stat);
    RESP_PUSH(0x20);

    cdrom->delayed_command = CDL_NONE;
    cdrom->state = CD_STATE_CMD;
}
void cdrom_cmd_unimplemented(psx_cdrom_t* cdrom) {
    log_fatal("Unimplemented CDROM command");

    exit(1);
}
void cdrom_cmd_getstat(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlGetStat: Expected exactly 0 parameters");

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_GETSTAT;
            cdrom->state = CD_STATE_RESP1;
        } break;

        case CD_STATE_RESP1: {
            cdrom->delayed_command = CDL_NONE;

            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(cdrom->stat);

            cdrom->state = CD_STATE_CMD;
        } break;
    }
}
void cdrom_cmd_setloc(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            if (cdrom->pfifo_index != 3) {
                log_fatal("CdlSetloc: Expected exactly 3 parameters, got %u instead",
                    cdrom->pfifo_index
                );

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_SETLOC;
            cdrom->state = CD_STATE_RESP1;

            cdrom->seek_sect = cdrom_btoi(PFIFO_POP);
            cdrom->seek_ss = cdrom_btoi(PFIFO_POP);
            cdrom->seek_mm = cdrom_btoi(PFIFO_POP);

            // Account for 2 second gap
            uint32_t seconds = (cdrom->seek_mm * 60) + cdrom->seek_ss - 2;
            uint32_t sectors = (seconds * CD_SECTORS_PER_SECOND) + cdrom->seek_sect;

            cdrom->seek_offset = sectors * CD_SECTOR_SIZE;

            log_fatal("setloc: %02u:%02u:%02u (%08x)",
                cdrom->seek_mm,
                cdrom->seek_ss,
                cdrom->seek_sect,
                cdrom->seek_offset
            );
        } break;

        case CD_STATE_RESP1: {
            cdrom->delayed_command = CDL_NONE;

            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(cdrom->stat);

            cdrom->state = CD_STATE_CMD;
        } break;
    }
}
void cdrom_cmd_readn(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            log_fatal("CdlReadN: CD_STATE_CMD");
            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_RESP1;
            cdrom->delayed_command = CDL_READN;
        } break;

        case CD_STATE_RESP1: {
            log_fatal("CdlReadN: CD_STATE_RESP1");

            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(GETSTAT_MOTOR);

            fseek(cdrom->disc, cdrom->seek_offset, 0);

            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = double_speed ? READ_DOUBLE_DELAY : READ_SINGLE_DELAY;
            cdrom->state = CD_STATE_RESP2;
            cdrom->delayed_command = CDL_READN;

            if (cdrom->spin_delay) {
                cdrom->irq_delay += cdrom->spin_delay;
                cdrom->spin_delay = 0;
            }
        } break;

        case CD_STATE_RESP2: {
            log_fatal("CdlReadN: CD_STATE_RESP2");

            SET_BITS(ifr, IFR_INT, 1);
            RESP_PUSH(GETSTAT_MOTOR | GETSTAT_READ);

            log_fatal("Reading data from disc. offset=%02x:%02x:%02x (%08x, tellg=%08x)",
                cdrom->seek_mm, cdrom->seek_ss, cdrom->seek_sect,
                cdrom->seek_offset, ftell(cdrom->disc)
            );

            cdrom->dfifo_index = 0;

            fread(cdrom->dfifo, 1, CD_SECTOR_SIZE, cdrom->disc);

            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = double_speed ? READ_DOUBLE_DELAY : READ_SINGLE_DELAY;
            cdrom->state = CD_STATE_RESP2;
            cdrom->delayed_command = CDL_READN;
        } break;
    }
}
void cdrom_cmd_stop(psx_cdrom_t* cdrom) { log_fatal("stop: Unimplemented"); exit(1); }
void cdrom_cmd_pause(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_RESP1;
            cdrom->delayed_command = CDL_PAUSE;
        } break;

        case CD_STATE_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(GETSTAT_MOTOR | GETSTAT_READ);

            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = DELAY_1MS * (double_speed ? 70 : 35);
            cdrom->state = CD_STATE_RESP2;
            cdrom->delayed_command = CDL_PAUSE;
        } break;

        case CD_STATE_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->state = CD_STATE_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_init(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_RESP1;
            cdrom->delayed_command = CDL_INIT;
        } break;

        case CD_STATE_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(cdrom->stat);

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_RESP2;
            cdrom->delayed_command = CDL_INIT;
        } break;

        case CD_STATE_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(cdrom->stat);

            cdrom->state = CD_STATE_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_unmute(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlGetStat: Expected exactly 0 parameters");

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_DEMUTE;
            cdrom->state = CD_STATE_RESP1;
        } break;

        case CD_STATE_RESP1: {
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(cdrom->stat);

            cdrom->delayed_command = CDL_NONE;
            cdrom->state = CD_STATE_CMD;
        } break;
    }
}
void cdrom_cmd_setfilter(psx_cdrom_t* cdrom) { log_fatal("setfilter: Unimplemented"); exit(1); }
void cdrom_cmd_setmode(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            if (cdrom->pfifo_index != 1) {
                log_fatal("CdlSetmode: Expected exactly 1 parameter");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;

                return;
            }

            int prev_speed = cdrom->mode & MODE_SPEED;

            cdrom->mode = PFIFO_POP;

            if ((cdrom->mode & MODE_SPEED) != prev_speed)
                cdrom->spin_delay = DELAY_1MS * 650;

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_SETMODE;
            cdrom->state = CD_STATE_RESP1;
        } break;

        case CD_STATE_RESP1: {
            cdrom->delayed_command = CDL_NONE;
    
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(cdrom->stat);

            cdrom->state = CD_STATE_CMD;
        } break;
    }
}
void cdrom_cmd_getlocl(psx_cdrom_t* cdrom) { log_fatal("getlocl: Unimplemented"); exit(1); }
void cdrom_cmd_getlocp(psx_cdrom_t* cdrom) { log_fatal("getlocp: Unimplemented"); exit(1); }
void cdrom_cmd_gettn(psx_cdrom_t* cdrom) { log_fatal("gettn: Unimplemented"); exit(1); }
void cdrom_cmd_gettd(psx_cdrom_t* cdrom) { log_fatal("gettd: Unimplemented"); exit(1); }
void cdrom_cmd_seekl(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_RESP1;
            cdrom->delayed_command = CDL_SEEKL;

            log_fatal("seekl: Seeking to address %08x", cdrom->seek_offset);

            fseek(cdrom->disc, cdrom->seek_offset, 0);
        } break;

        case CD_STATE_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(cdrom->stat);

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_RESP2;
            cdrom->delayed_command = CDL_SEEKL;
        } break;

        case CD_STATE_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(cdrom->stat);

            cdrom->state = CD_STATE_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_seekp(psx_cdrom_t* cdrom) { log_fatal("seekp: Unimplemented"); exit(1); }
void cdrom_cmd_test(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            if (cdrom->pfifo_index != 1) {
                log_fatal("CdlTest: Expected exactly 1 parameter");

                return;
            }

            if (PFIFO_POP != 0x20) {
                log_fatal("CdlTest: Unhandled subcommand");

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_TEST;
            cdrom->state = CD_STATE_RESP1;
        } break;

        case CD_STATE_RESP1: {
            cdrom->delayed_command = CDL_NONE;
    
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(0x01);
            RESP_PUSH(0x95);
            RESP_PUSH(0x13);
            RESP_PUSH(0x03);

            cdrom->state = CD_STATE_CMD;
        } break;
    }
}
void cdrom_cmd_getid(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlGetID: Expected exactly 0 parameters");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_RESP1;
            cdrom->delayed_command = CDL_GETID;
        } break;

        case CD_STATE_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(cdrom->stat);

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_RESP2;
            cdrom->delayed_command = CDL_GETID;
        } break;

        case CD_STATE_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);

            if (cdrom->disc) {
                RESP_PUSH(0x41);
                RESP_PUSH(0x45);
                RESP_PUSH(0x43);
                RESP_PUSH(0x53);
                RESP_PUSH(0x00);
                RESP_PUSH(0x20);
                RESP_PUSH(0x00);
                RESP_PUSH(0x02);
            } else {
                RESP_PUSH(0x00);
                RESP_PUSH(0x00);
                RESP_PUSH(0x00);
                RESP_PUSH(0x00);
                RESP_PUSH(0x00);
                RESP_PUSH(0x00);
                RESP_PUSH(0x40);
                RESP_PUSH(0x08);
            }

            cdrom->state = CD_STATE_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_reads(psx_cdrom_t* cdrom) { log_fatal("reads: Unimplemented"); exit(1); }
void cdrom_cmd_readtoc(psx_cdrom_t* cdrom) { log_fatal("readtoc: Unimplemented"); exit(1); }

typedef void (*cdrom_cmd_t)(psx_cdrom_t*);

const char* g_psx_cdrom_command_names[] = {
    "CdlUnimplemented",
    "CdlGetstat",
    "CdlSetloc",
    "CdlUnimplemented",
    "CdlUnimplemented",
    "CdlUnimplemented",
    "CdlReadn",
    "CdlUnimplemented",
    "CdlStop",
    "CdlPause",
    "CdlInit",
    "CdlUnimplemented",
    "CdlUnmute",
    "CdlSetfilter",
    "CdlSetmode",
    "CdlUnimplemented",
    "CdlGetlocl",
    "CdlGetlocp",
    "CdlUnimplemented",
    "CdlGettn",
    "CdlGettd",
    "CdlSeekl",
    "CdlSeekp",
    "CdlUnimplemented",
    "CdlUnimplemented",
    "CdlTest",
    "CdlGetid",
    "CdlReads",
    "CdlUnimplemented",
    "CdlUnimplemented",
    "CdlReadtoc",
    "CdlUnimplemented"
};

cdrom_cmd_t g_psx_cdrom_command_table[] = {
    cdrom_cmd_unimplemented,
    cdrom_cmd_getstat,
    cdrom_cmd_setloc,
    cdrom_cmd_unimplemented,
    cdrom_cmd_unimplemented,
    cdrom_cmd_unimplemented,
    cdrom_cmd_readn,
    cdrom_cmd_unimplemented,
    cdrom_cmd_stop,
    cdrom_cmd_pause,
    cdrom_cmd_init,
    cdrom_cmd_unimplemented,
    cdrom_cmd_unmute,
    cdrom_cmd_setfilter,
    cdrom_cmd_setmode,
    cdrom_cmd_unimplemented,
    cdrom_cmd_getlocl,
    cdrom_cmd_getlocp,
    cdrom_cmd_unimplemented,
    cdrom_cmd_gettn,
    cdrom_cmd_gettd,
    cdrom_cmd_seekl,
    cdrom_cmd_seekp,
    cdrom_cmd_unimplemented,
    cdrom_cmd_unimplemented,
    cdrom_cmd_test,
    cdrom_cmd_getid,
    cdrom_cmd_reads,
    cdrom_cmd_unimplemented,
    cdrom_cmd_unimplemented,
    cdrom_cmd_readtoc,
    cdrom_cmd_unimplemented
};

typedef uint8_t (*psx_cdrom_read_function_t)(psx_cdrom_t*);
typedef void (*psx_cdrom_write_function_t)(psx_cdrom_t*, uint8_t);

uint8_t cdrom_read_status(psx_cdrom_t* cdrom) {
    return cdrom->status;
}

uint8_t cdrom_read_rfifo(psx_cdrom_t* cdrom) {
    uint8_t data = cdrom->rfifo[--cdrom->rfifo_index];

    if (cdrom->rfifo_index == 0)
        SET_BITS(status, STAT_RSLRRDY_MASK, 0);

    return data;
}

uint8_t cdrom_read_dfifo(psx_cdrom_t* cdrom) {
    if (!cdrom->dfifo_full)
        return 0;

    int sector_size_bit = cdrom->mode & MODE_SECTOR_SIZE;

    uint32_t read_sector_size = sector_size_bit ? 0x924 : 0x800;
    uint32_t offset = sector_size_bit ? 12 : 24;
    
    if (cdrom->dfifo_index != read_sector_size) {
        SET_BITS(status, STAT_DRQSTS_MASK, STAT_DRQSTS_MASK);

        return cdrom->dfifo[offset + (cdrom->dfifo_index++)];
    } else {
        SET_BITS(status, STAT_DRQSTS_MASK, 0);

        cdrom->dfifo_full = 0;
    }

    return 0x00;
}

uint8_t cdrom_read_ier(psx_cdrom_t* cdrom) {
    return cdrom->ier;
}

uint8_t cdrom_read_ifr(psx_cdrom_t* cdrom) {
    return 0xe0 | cdrom->ifr;
}

void cdrom_write_status(psx_cdrom_t* cdrom, uint8_t value) {
    SET_BITS(status, STAT_INDEX_MASK, value);
}

void cdrom_write_cmd(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("%s(%02x) %u params=[%02x, %02x, %02x, %02x, %02x, %02x]",
        g_psx_cdrom_command_names[value],
        value,
        cdrom->pfifo_index,
        cdrom->pfifo[0],
        cdrom->pfifo[1],
        cdrom->pfifo[2],
        cdrom->pfifo[3],
        cdrom->pfifo[4],
        cdrom->pfifo[5]
    );

    cdrom->state = CD_STATE_CMD;
    cdrom->delayed_command = CDL_NONE;

    g_psx_cdrom_command_table[value](cdrom);
}

void cdrom_write_pfifo(psx_cdrom_t* cdrom, uint8_t value) {
    cdrom->pfifo[(cdrom->pfifo_index++) & 0xf] = value;

    SET_BITS(status, STAT_PRMWRDY_MASK, (cdrom->pfifo_index & 0x10) ? 0x0 : 0xff);

    cdrom->pfifo_index &= 0x1f;
}

void cdrom_write_req(psx_cdrom_t* cdrom, uint8_t value) {
    cdrom->request = value;

    if (cdrom->request & REQ_BFRD) {
        SET_BITS(status, STAT_DRQSTS_MASK, STAT_DRQSTS_MASK);

        cdrom->dfifo_full = 1;
    } else {
        SET_BITS(status, STAT_DRQSTS_MASK, 0);

        cdrom->dfifo_full = 0;
        cdrom->dfifo_index = 0;
    }
}

void cdrom_write_smdout(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Sound map data out unimplemented");
}

void cdrom_write_ier(psx_cdrom_t* cdrom, uint8_t value) {
    cdrom->ier = value;
}

void cdrom_write_ifr(psx_cdrom_t* cdrom, uint8_t value) {
    cdrom->ifr &= ~(value & 0x7);

    // Clear Parameter FIFO
    if (value & 0x40) {
        cdrom->pfifo_index = 0;

        SET_BITS(
            status,
            (STAT_PRMEMPT_MASK | STAT_PRMWRDY_MASK),
            (STAT_PRMEMPT_MASK | STAT_PRMWRDY_MASK)
        );
    }

    // Clear Response FIFO
    // if (value & 0x7) {
    //     cdrom->rfifo_index = 0;

    //     SET_BITS(status, STAT_RSLRRDY_MASK, 0);
    // }
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

const char* g_psx_cdrom_read_names_table[] = {
    "cdrom_read_status", "cdrom_read_rfifo", "cdrom_read_dfifo", "cdrom_read_ier",
    "cdrom_read_status", "cdrom_read_rfifo", "cdrom_read_dfifo", "cdrom_read_ifr",
    "cdrom_read_status", "cdrom_read_rfifo", "cdrom_read_dfifo", "cdrom_read_ier",
    "cdrom_read_status", "cdrom_read_rfifo", "cdrom_read_dfifo", "cdrom_read_ifr"
};

const char* g_psx_cdrom_write_names_table[] = {
    "cdrom_write_status", "cdrom_write_cmd"     , "cdrom_write_pfifo"   , "cdrom_write_req"     ,
    "cdrom_write_status", "cdrom_write_smdout"  , "cdrom_write_ier"     , "cdrom_write_ifr"     ,
    "cdrom_write_status", "cdrom_write_sminfo"  , "cdrom_write_lcdlspuv", "cdrom_write_lcdrspuv",
    "cdrom_write_status", "cdrom_write_rcdrspuv", "cdrom_write_rcdlspuv", "cdrom_write_volume"
};

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
    uint8_t data = g_psx_cdrom_read_table[(STAT_INDEX << 2) | offset](cdrom);

    // log_fatal("%s (read %02x)", g_psx_cdrom_read_names_table[(STAT_INDEX << 2) | offset], data);

    return data;
}

void psx_cdrom_write32(psx_cdrom_t* cdrom, uint32_t offset, uint32_t value) {
    log_fatal("Unhandled 32-bit CDROM write at offset %08x (%08x)", offset, value);
}

void psx_cdrom_write16(psx_cdrom_t* cdrom, uint32_t offset, uint16_t value) {
    log_fatal("Unhandled 16-bit CDROM write at offset %08x (%04x)", offset, value);
}

void psx_cdrom_write8(psx_cdrom_t* cdrom, uint32_t offset, uint8_t value) {
    // log_fatal("%s (write %02x)", g_psx_cdrom_write_names_table[(STAT_INDEX << 2) | offset], value);

    g_psx_cdrom_write_table[(STAT_INDEX << 2) | offset](cdrom, value);
}

void psx_cdrom_update(psx_cdrom_t* cdrom) {
    if (cdrom->irq_delay) {
        cdrom->irq_delay -= 2;

        if (cdrom->irq_delay <= 0) {
            if (cdrom->int_number) {
                SET_BITS(ifr, IFR_INT, cdrom->int_number);

                cdrom->int_number = 0;
            }

            log_fatal("CDROM INT%u", cdrom->ifr & 0x7);
            psx_ic_irq(cdrom->ic, IC_CDROM);

            cdrom->irq_delay = 0;

            if (cdrom->delayed_command) {
                g_psx_cdrom_command_table[cdrom->delayed_command](cdrom);
            }
        }
    }
}

void psx_cdrom_open(psx_cdrom_t* cdrom, const char* path) {
    cdrom->disc = fopen(path, "rb");

    if (!cdrom->disc) {
        log_fatal("Couldn't open disc image \"%s\"", path);

        cdrom->stat |= GETSTAT_TRAYOPEN;

        return;
    }

    fseek(cdrom->disc, 0, 0);
}

void psx_cdrom_close(psx_cdrom_t* cdrom) {
    if (cdrom->disc)
        fclose(cdrom->disc);
}

void psx_cdrom_destroy(psx_cdrom_t* cdrom) {
    free(cdrom);
}