#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cdrom.h"
#include "../log.h"

#define CPU_SPEED (44100 * 0x300)
#define COMMAND_DELAY (CPU_SPEED / 1000)
#define READ_SINGLE_DELAY (CPU_SPEED / 75)
#define READ_DOUBLE_DELAY (CPU_SPEED / (2 * 75))

#define RESP_PUSH(data) \
    cdrom->rfifo[cdrom->rfifo_index++] = data; \
    SET_BITS(status, STAT_RSLRRDY_MASK, STAT_RSLRRDY_MASK);

#define SEND_INT1(delay) SET_BITS(ifr, IFR_INT, IFR_INT1) cdrom->irq_delay = delay;
#define SEND_INT2(delay) SET_BITS(ifr, IFR_INT, IFR_INT2) cdrom->irq_delay = delay;
#define SEND_INT3(delay) SET_BITS(ifr, IFR_INT, IFR_INT3) cdrom->irq_delay = delay;
#define SEND_INT4(delay) SET_BITS(ifr, IFR_INT, IFR_INT4) cdrom->irq_delay = delay;
#define SEND_INT5(delay) SET_BITS(ifr, IFR_INT, IFR_INT5) cdrom->irq_delay = delay;
#define SEND_INT6(delay) SET_BITS(ifr, IFR_INT, IFR_INT6) cdrom->irq_delay = delay;
#define SEND_INT7(delay) SET_BITS(ifr, IFR_INT, IFR_INT7) cdrom->irq_delay = delay;

uint8_t cdrom_btoi(uint8_t b) {
    return ((b >> 4) * 10) + (b & 0xf);
}

void cdrom_cmd_readn(psx_cdrom_t* cdrom) {
    if (!cdrom->delayed_response_command) {
        RESP_PUSH(cdrom->stat);
        SEND_INT3(COMMAND_DELAY);

        cdrom->stat |= GETSTAT_READ;

        cdrom->delayed_response_command = cdrom->command;
    } else {
        log_fatal("Reading data from disc. offset=%02x:%02x:%02x (%08x)",
            cdrom->seek_mm, cdrom->seek_ss, cdrom->seek_sect,
            cdrom->seek_offset
        );

        fread(cdrom->read_buf, 1, CD_SECTOR_SIZE, cdrom->disc);

        int double_speed = cdrom->mode & MODE_SPEED;

        RESP_PUSH(cdrom->stat);
        SEND_INT1(double_speed ? READ_DOUBLE_DELAY : READ_SINGLE_DELAY);

        cdrom->dfifo_full = 1;

        cdrom->stat &= ~GETSTAT_READ;

        // Repeat until pause
        cdrom->delayed_response_command = cdrom->command;
    }
}

void cdrom_cmd_stop(psx_cdrom_t* cdrom) {
    log_fatal("stop: Unimplemented CD command");
}

void cdrom_cmd_pause(psx_cdrom_t* cdrom) {
    log_fatal("pause command");
    if (!cdrom->delayed_response_command) {
        RESP_PUSH(cdrom->stat);
        SEND_INT3(COMMAND_DELAY);

        cdrom->delayed_response_command = cdrom->command;
    } else {
        RESP_PUSH(cdrom->stat);
        SEND_INT2(COMMAND_DELAY);

        cdrom->delayed_response_command = 0;
    }
}

void cdrom_cmd_init(psx_cdrom_t* cdrom) {
    if (!cdrom->delayed_response_command) {
        RESP_PUSH(cdrom->stat);
        SEND_INT3(COMMAND_DELAY);

        cdrom->delayed_response_command = cdrom->command;
    } else {
        RESP_PUSH(cdrom->stat);
        SEND_INT2(COMMAND_DELAY);

        cdrom->delayed_response_command = 0;
    }
}

void cdrom_cmd_unmute(psx_cdrom_t* cdrom) {
    RESP_PUSH(cdrom->stat);
    SEND_INT3(COMMAND_DELAY);
}

void cdrom_cmd_setfilter(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("setfilter: Unimplemented CD command");
    log_set_quiet(1);
}

void cdrom_cmd_setmode(psx_cdrom_t* cdrom) {
    if (cdrom->pfifo_index != 1) {
        log_fatal("setmode: Expected exactly 1 parameter");

        return;
    }

    cdrom->mode = cdrom->pfifo[--cdrom->pfifo_index];

    RESP_PUSH(cdrom->stat);
    SEND_INT3(COMMAND_DELAY);
}

void cdrom_cmd_getlocl(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("getlocl: Unimplemented CD command");
    log_set_quiet(1);
}

void cdrom_cmd_getlocp(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("getlocp: Unimplemented CD command");
    log_set_quiet(1);
}

void cdrom_cmd_gettn(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("gettn: Unimplemented CD command");
    log_set_quiet(1);
}

void cdrom_cmd_gettd(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("gettd: Unimplemented CD command");
    log_set_quiet(1);
}

void cdrom_cmd_seekl(psx_cdrom_t* cdrom) {
    if (!cdrom->delayed_response_command) {
        RESP_PUSH(cdrom->stat);
        SEND_INT3(COMMAND_DELAY);

        log_fatal("seekl: Seeking to address %08x", cdrom->seek_offset);

        fseek(cdrom->disc, cdrom->seek_offset, 0);

        cdrom->delayed_response_command = cdrom->command;
    } else {
        RESP_PUSH(cdrom->stat);
        SEND_INT2(COMMAND_DELAY);

        cdrom->delayed_response_command = 0;
    }
}

void cdrom_cmd_seekp(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("seekp: Unimplemented CD command");
    log_set_quiet(1);
}

void cdrom_cmd_reads(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("reads: Unimplemented CD command");
    log_set_quiet(1);
}

void cdrom_cmd_readtoc(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("readtoc: Unimplemented CD command");
    log_set_quiet(1);
}

void cdrom_cmd_unimplemented(psx_cdrom_t* cdrom) {
    log_set_quiet(0);
    log_fatal("Unimplemented CD command %02x", cdrom->command);
    log_set_quiet(1);
}

void cdrom_cmd_getstat(psx_cdrom_t* cdrom) {
    if (!cdrom->tray_open) {
        SET_BITS(stat, GETSTAT_TRAYOPEN, 0);
    }

    RESP_PUSH(cdrom->stat);
    SEND_INT3(COMMAND_DELAY);
}

void cdrom_cmd_setloc(psx_cdrom_t* cdrom) {
    if (cdrom->pfifo_index != 3) {
        log_fatal("setloc: Expected exactly 3 parameters, got %u instead", cdrom->pfifo_index);

        return;
    }

    cdrom->seek_sect = cdrom_btoi(cdrom->pfifo[--cdrom->pfifo_index]);
    cdrom->seek_ss = cdrom_btoi(cdrom->pfifo[--cdrom->pfifo_index]);
    cdrom->seek_mm = cdrom_btoi(cdrom->pfifo[--cdrom->pfifo_index]);

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

    RESP_PUSH(cdrom->stat);
    SEND_INT3(COMMAND_DELAY);
}

void cdrom_cmd_getid(psx_cdrom_t* cdrom) {
    if (!cdrom->delayed_response_command) {
        // RESP_PUSH(cdrom->stat);
        SEND_INT3(COMMAND_DELAY);

        cdrom->delayed_response_command = cdrom->command;
    } else {
        if (!cdrom->disc) {
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x40);
            RESP_PUSH(0x08);
        } else {
            RESP_PUSH('A');
            RESP_PUSH('E');
            RESP_PUSH('C');
            RESP_PUSH('S');
            RESP_PUSH(0x00);
            RESP_PUSH(0x20);
            RESP_PUSH(0x00);
            RESP_PUSH(0x02);
        }

        SEND_INT2(COMMAND_DELAY);

        cdrom->delayed_response_command = 0;
    }
}

void cdrom_cmd_test(psx_cdrom_t* cdrom) {
    if (cdrom->pfifo_index != 1) {
        log_fatal("test: Expected exactly 1 parameter");
    }

    switch (cdrom->pfifo[--cdrom->pfifo_index]) {
        case 0x20: {
            RESP_PUSH(0x01);
            RESP_PUSH(0x95);
            RESP_PUSH(0x13);
            RESP_PUSH(0x03);
            SEND_INT3(COMMAND_DELAY);
        } break;
    }
}

typedef void (*cdrom_cmd_t)(psx_cdrom_t*);

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

/*
  Command          Parameters      Response(s)
  00h -            -               INT5(11h,40h)  ;reportedly "Sync" uh?
  01h Getstat      -               INT3(stat)
  02h Setloc     E amm,ass,asect   INT3(stat)
  03h Play       E (track)         INT3(stat), optional INT1(report bytes)
  04h Forward    E -               INT3(stat), optional INT1(report bytes)
  05h Backward   E -               INT3(stat), optional INT1(report bytes)
  06h ReadN      E -               INT3(stat), INT1(stat), datablock
  07h MotorOn    E -               INT3(stat), INT2(stat)
  08h Stop       E -               INT3(stat), INT2(stat)
  09h Pause      E -               INT3(stat), INT2(stat)
  0Ah Init         -               INT3(late-stat), INT2(stat)
  0Bh Mute       E -               INT3(stat)
  0Ch Demute     E -               INT3(stat)
  0Dh Setfilter  E file,channel    INT3(stat)
  0Eh Setmode      mode            INT3(stat)
  0Fh Getparam     -               INT3(stat,mode,null,file,channel)
  10h GetlocL    E -               INT3(amm,ass,asect,mode,file,channel,sm,ci)
  11h GetlocP    E -               INT3(track,index,mm,ss,sect,amm,ass,asect)
  12h SetSession E session         INT3(stat), INT2(stat)
  13h GetTN      E -               INT3(stat,first,last)  ;BCD
  14h GetTD      E track (BCD)     INT3(stat,mm,ss)       ;BCD
  15h SeekL      E -               INT3(stat), INT2(stat)  ;\use prior Setloc
  16h SeekP      E -               INT3(stat), INT2(stat)  ;/to set target
  17h -            -               INT5(11h,40h)  ;reportedly "SetClock" uh?
  18h -            -               INT5(11h,40h)  ;reportedly "GetClock" uh?
  19h Test         sub_function    depends on sub_function (see below)
  1Ah GetID      E -               INT3(stat), INT2/5(stat,flg,typ,atip,"SCEx")
  1Bh ReadS      E?-               INT3(stat), INT1(stat), datablock
  1Ch Reset        -               INT3(stat), Delay            ;-not DTL-H2000
  1Dh GetQ       E adr,point       INT3(stat), INT2(10bytesSubQ,peak_lo) ;\not
  1Eh ReadTOC      -               INT3(late-stat), INT2(stat)           ;/vC0
  1Fh VideoCD      sub,a,b,c,d,e   INT3(stat,a,b,c,d,e)   ;<-- SCPH-5903 only
  1Fh..4Fh -       -               INT5(11h,40h)  ;-Unused/invalid
  50h Secret 1     -               INT5(11h,40h)  ;\
  51h Secret 2     "Licensed by"   INT5(11h,40h)  ;
  52h Secret 3     "Sony"          INT5(11h,40h)  ; Secret Unlock Commands
  53h Secret 4     "Computer"      INT5(11h,40h)  ; (not in version vC0, and,
  54h Secret 5     "Entertainment" INT5(11h,40h)  ; nonfunctional in japan)
  55h Secret 6     "<region>"      INT5(11h,40h)  ;
  56h Secret 7     -               INT5(11h,40h)  ;/
  57h SecretLock   -               INT5(11h,40h)  ;-Secret Lock Command
  58h..5Fh Crash   -               Crashes the HC05 (jumps into a data area)
  6Fh..FFh -       -               INT5(11h,40h)  ;-Unused/invalid
*/

typedef uint8_t (*psx_cdrom_read_function_t)(psx_cdrom_t*);
typedef void (*psx_cdrom_write_function_t)(psx_cdrom_t*, uint8_t);

uint8_t cdrom_read_status(psx_cdrom_t* cdrom) {
    //log_fatal("    Status read %02x, pfifo_index=%u", cdrom->status, cdrom->pfifo_index);

    return cdrom->status;
}

uint8_t cdrom_read_rfifo(psx_cdrom_t* cdrom) {
    if (!cdrom->rfifo_index) {
        uint8_t data = cdrom->rfifo[cdrom->rfifo_index];
        
        SET_BITS(status, STAT_RSLRRDY_MASK, 0);

        //log_fatal("    RFIFO read (%02x)", data);

        return data;
    } else {
        uint8_t data = cdrom->rfifo[--cdrom->rfifo_index];
    
        //log_fatal("    RFIFO read (%02x)", data);
    
        return data;
    }
}

uint8_t cdrom_read_dfifo(psx_cdrom_t* cdrom) {
    if (!cdrom->dfifo_full)
        return 0x00;

    int sector_size_bit = cdrom->mode & MODE_SECTOR_SIZE;

    uint32_t read_sector_size = sector_size_bit ? 0x924 : 0x800;
    uint32_t offset = sector_size_bit ? 12 : 24;
    
    if (cdrom->dfifo_index != read_sector_size) {
        return cdrom->dfifo[offset + (cdrom->dfifo_index++)];
    } else {
        cdrom->dfifo_full = 0;
    }

    return 0x00;
}

uint8_t cdrom_read_ier(psx_cdrom_t* cdrom) {
    //log_fatal("    IER read %02x", cdrom->ier);

    return cdrom->ier;
}

uint8_t cdrom_read_ifr(psx_cdrom_t* cdrom) {
    //log_fatal("    IFR read %02x", cdrom->ifr);

    return cdrom->ifr;
}

void cdrom_write_status(psx_cdrom_t* cdrom, uint8_t value) {
    //log_fatal("    Status write %02x, pfifo_index=%u", value, cdrom->pfifo_index);

    SET_BITS(status, STAT_INDEX_MASK, value);
}

void cdrom_write_cmd(psx_cdrom_t* cdrom, uint8_t value) {
    cdrom->delayed_response_command = 0;
    cdrom->command = value;

    g_psx_cdrom_command_table[value](cdrom);

    log_fatal("    Command %02x (pfifo=%02x, %02x, %02x, %02x), pfifo_index=%u",
        value,
        cdrom->pfifo[0],
        cdrom->pfifo[1],
        cdrom->pfifo[2],
        cdrom->pfifo[3],
        cdrom->pfifo_index
    );
}

void cdrom_write_pfifo(psx_cdrom_t* cdrom, uint8_t value) {
    cdrom->pfifo[(cdrom->pfifo_index++) & 0xf] = value;

    SET_BITS(status, STAT_PRMWRDY_MASK, (cdrom->pfifo_index & 0x10) ? 0x0 : 0xff);

    cdrom->pfifo_index &= 0x1f;
}

void cdrom_write_req(psx_cdrom_t* cdrom, uint8_t value) {
    cdrom->request = value;

    if (cdrom->request & REQ_BFRD) {
        memcpy(cdrom->dfifo, cdrom->read_buf, CD_SECTOR_SIZE);

        cdrom->dfifo_full = 1;
    } else {
        cdrom->dfifo_full = 0;
        cdrom->dfifo_index = 0;
    }
}

void cdrom_write_smdout(psx_cdrom_t* cdrom, uint8_t value) {
    log_fatal("Sound map data out unimplemented");
}

void cdrom_write_ier(psx_cdrom_t* cdrom, uint8_t value) {
    //log_fatal("    IER write %02x", value);

    cdrom->ier = value;
}

void cdrom_write_ifr(psx_cdrom_t* cdrom, uint8_t value) {
    //log_fatal("    IFR write %02x", value);

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

        if (cdrom->irq_delay <= 0) {
            psx_ic_irq(cdrom->ic, IC_CDROM);

            cdrom->irq_delay = 0;

            log_fatal("delayed_command=%02x", cdrom->delayed_response_command);

            if (cdrom->delayed_response_command) {
                g_psx_cdrom_command_table[cdrom->delayed_response_command](cdrom);
                log_fatal("Delayed execution delay=%08x cmd=%02x", cdrom->irq_delay, cdrom->delayed_response_command);
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