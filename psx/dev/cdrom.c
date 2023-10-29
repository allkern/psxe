#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cdrom.h"
#include "../log.h"
#include "../msf.h"

/*
    Drive Status           1st Response   2nd Response
    Door Open              INT5(11h,80h)  N/A
    Spin-up                INT5(01h,80h)  N/A
    Detect busy            INT5(03h,80h)  N/A
    No Disk                INT3(stat)     INT5(08h,40h, 00h,00h, 00h,00h,00h,00h)
    Audio Disk             INT3(stat)     INT5(0Ah,90h, 00h,00h, 00h,00h,00h,00h)
    Unlicensed:Mode1       INT3(stat)     INT5(0Ah,80h, 00h,00h, 00h,00h,00h,00h)
    Unlicensed:Mode2       INT3(stat)     INT5(0Ah,80h, 20h,00h, 00h,00h,00h,00h)
    Unlicensed:Mode2+Audio INT3(stat)     INT5(0Ah,90h, 20h,00h, 00h,00h,00h,00h)
    Debug/Yaroze:Mode2     INT3(stat)     INT2(02h,00h, 20h,00h, 20h,20h,20h,20h)
    Licensed:Mode2         INT3(stat)     INT2(02h,00h, 20h,00h, 53h,43h,45h,4xh)
    Modchip:Audio/Mode1    INT3(stat)     INT2(02h,00h, 00h,00h, 53h,43h,45h,4xh)
*/

#define GETID_RESPONSE_SIZE 8
#define GETID_RESPONSE_END (GETID_RESPONSE_SIZE - 1)

static const uint8_t g_getid_no_disc[] = {
    0x08, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static const uint8_t g_getid_audio[] = {
    0x0a, 0x90, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static const uint8_t g_getid_unlicensed[] = {
    0x0a, 0x80, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static const uint8_t g_getid_licensed[] = {
    0x02, 0x00, 0x20, 0x00,
    'S' , 'C' , 'E' , 'A'
};

#define RESP_PUSH(data) { \
    cdrom->rfifo[cdrom->rfifo_index++] = data; \
    cdrom->rfifo_index &= 15; \
    SET_BITS(status, STAT_RSLRRDY_MASK, STAT_RSLRRDY_MASK); }

#define PFIFO_POP (cdrom->pfifo[--cdrom->pfifo_index])

#define VALID_BCD(v) (((v & 0xf) <= 9) && ((v & 0xf0) <= 0x90))

void cdrom_cmd_error(psx_cdrom_t* cdrom) {
    SET_BITS(ifr, IFR_INT, IFR_INT5);
    RESP_PUSH(cdrom->error);
    RESP_PUSH(GETSTAT_MOTOR | cdrom->error_flags);

    cdrom->pfifo_index = 0;
    cdrom->delayed_command = CDL_NONE;
    cdrom->state = CD_STATE_RECV_CMD;
}
void cdrom_cmd_unimplemented(psx_cdrom_t* cdrom) {
    log_fatal("Unimplemented CDROM command (%u)", cdrom->command);

    exit(1);
}
void cdrom_cmd_getstat(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlGetStat: Expected exactly 0 parameters");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_GETSTAT;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(GETSTAT_MOTOR | (cdrom->disc ? 0 : GETSTAT_TRAYOPEN));

            if (cdrom->read_ongoing) {
                cdrom->state = CD_STATE_SEND_RESP2;
                cdrom->delayed_command = CDL_READN;
                cdrom->irq_delay = DELAY_1MS;
            } else {
                cdrom->delayed_command = CDL_NONE;
                cdrom->state = CD_STATE_RECV_CMD;
            }
        } break;
    }
}
void cdrom_cmd_setloc(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index != 3) {
                log_fatal("CdlSetloc: Expected exactly 3 parameters, got %u instead",
                    cdrom->pfifo_index
                );

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            if (!cdrom->read_ongoing) {
                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_SETLOC;
                cdrom->state = CD_STATE_SEND_RESP1;
            } else {
                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_READN;
                cdrom->state = CD_STATE_SEND_RESP2;
            }

            int f = PFIFO_POP;
            int s = PFIFO_POP;
            int m = PFIFO_POP;

            if (!(VALID_BCD(m) && VALID_BCD(s) && VALID_BCD(f) && (f < 0x75))) {
                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_INVSUBF;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->seek_ff = f;
            cdrom->seek_ss = s;
            cdrom->seek_mm = m;

            cdrom->seek_pending = 1;

            log_fatal("setloc: %02x:%02x:%02x",
                cdrom->seek_mm,
                cdrom->seek_ss,
                cdrom->seek_ff
            );
        } break;

        case CD_STATE_SEND_RESP1: {
            cdrom->delayed_command = CDL_NONE;

            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->state = CD_STATE_RECV_CMD;
        } break;

        // Read ongoing
        case CD_STATE_SEND_RESP2: {
            int f = PFIFO_POP;
            int s = PFIFO_POP;
            int m = PFIFO_POP;

            if (!(VALID_BCD(m) && VALID_BCD(s) && VALID_BCD(f) && (f < 0x75))) {
                cdrom->read_ongoing = false;
                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_INVSUBF;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->seek_ff = f;
            cdrom->seek_ss = s;
            cdrom->seek_mm = m;
        } break;
    }
}
void cdrom_cmd_play(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            int track = 0;

            // Optional track number parameter
            if (cdrom->pfifo_index)
                track = PFIFO_POP;

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_PLAY;

            if (track) {

                psx_disc_get_track_addr(cdrom->disc, &cdrom->cdda_msf, track);

                msf_to_bcd(&cdrom->cdda_msf);

                cdrom->cdda_track = track;
                cdrom->seek_mm = cdrom->cdda_msf.m;
                cdrom->seek_ss = cdrom->cdda_msf.s;
                cdrom->seek_ff = cdrom->cdda_msf.f;
    
                cdrom->seek_pending = 1;
            }

            if (cdrom->seek_pending) {
                cdrom->seek_pending = 0;

                cdrom->cdda_msf.m = cdrom->seek_mm;
                cdrom->cdda_msf.s = cdrom->seek_ss;
                cdrom->cdda_msf.f = cdrom->seek_ff;

                // Convert seek to I
                msf_t msf = cdrom->cdda_msf;
                
                msf_from_bcd(&msf);

                // Seek to that address and read sector
                psx_disc_seek(cdrom->disc, msf);
                psx_disc_read_sector(cdrom->disc, cdrom->cdda_buf);

                // Increment sector
                msf_add_f(&msf, 1);
                msf_to_bcd(&msf);

                cdrom->cdda_msf = msf;
                cdrom->cdda_sector_offset = 0;
            }

            cdrom->cdda_playing = 1;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(GETSTAT_MOTOR | GETSTAT_PLAY);

            cdrom->delayed_command = CDL_NONE;
            cdrom->state = CD_STATE_RECV_CMD;
        } break;
    }
}
void cdrom_cmd_readn(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;
    cdrom->read_ongoing = 1;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            log_fatal("CdlReadN: CD_STATE_RECV_CMD");
            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_READN;
        } break;

        case CD_STATE_SEND_RESP1: {
            log_fatal("CdlReadN: CD_STATE_SEND_RESP1");

            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(GETSTAT_MOTOR);

            msf_t msf;

            msf.m = cdrom->seek_mm;
            msf.s = cdrom->seek_ss;
            msf.f = cdrom->seek_ff;

            msf_from_bcd(&msf);

            int err = psx_disc_seek(cdrom->disc, msf);

            if (err) {
                log_set_quiet(0);
                log_fatal("CdlReadN: Out of bounds seek");
                log_set_quiet(1);

                cdrom->irq_delay = DELAY_1MS * 600;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_INVSUBF;
                cdrom->error_flags = GETSTAT_SEEKERROR;

                return;
            }

            psx_disc_read_sector(cdrom->disc, cdrom->dfifo);

            msf_t sector_msf;

            sector_msf.m = cdrom->dfifo[0x0c];
            sector_msf.s = cdrom->dfifo[0x0d];
            sector_msf.f = cdrom->dfifo[0x0e];

            int correct_msf = (cdrom->seek_mm == sector_msf.m) && 
                              (cdrom->seek_ss == sector_msf.s) &&
                              (cdrom->seek_ff == sector_msf.f);
            
            // Most probably audio sector:
            // Purposefully constructed audio data could
            // circumvent this detection code, but it will work
            // for most intents and purposes 
            if (!correct_msf) {
                log_set_quiet(0);
                log_fatal("CdlReadN: Audio read");
                log_set_quiet(1);

                cdrom->irq_delay = DELAY_1MS * 600;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_SEEK;
                cdrom->error_flags = GETSTAT_SEEKERROR;

                return;
            }
            
            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = double_speed ? READ_DOUBLE_DELAY : READ_SINGLE_DELAY;
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_READN;

            if (cdrom->spin_delay) {
                cdrom->irq_delay += cdrom->spin_delay;
                cdrom->spin_delay = 0;
            }
        } break;

        case CD_STATE_SEND_RESP2: {
            log_fatal("CdlReadN: CD_STATE_SEND_RESP2");

            msf_t msf;

            msf.m = cdrom->seek_mm;
            msf.s = cdrom->seek_ss;
            msf.f = cdrom->seek_ff;

            msf_from_bcd(&msf);

            psx_disc_seek(cdrom->disc, msf);
            psx_disc_read_sector(cdrom->disc, cdrom->dfifo);

            // printf("Sector header: msf=%02x:%02x:%02x, mode=%02x, subheader=%02x,%02x,%02x,%02x\n",
            //     cdrom->dfifo[0x0c],
            //     cdrom->dfifo[0x0d],
            //     cdrom->dfifo[0x0e],
            //     cdrom->dfifo[0x0f],
            //     cdrom->dfifo[0x10],
            //     cdrom->dfifo[0x11],
            //     cdrom->dfifo[0x12],
            //     cdrom->dfifo[0x13]
            // );

            if (cdrom->dfifo[0x12] & 0x20) {
                log_fatal("Unimplemented XA Form2 Sector");

                // exit(1);
            }

            cdrom->seek_ff++;

            if ((cdrom->seek_ff & 0xF) == 10) { cdrom->seek_ff += 0x10; cdrom->seek_ff &= 0xF0; }
            if (cdrom->seek_ff == 0x75) { cdrom->seek_ss++; cdrom->seek_ff = 0; }
            if ((cdrom->seek_ss & 0xF) == 10) { cdrom->seek_ss += 0x10; cdrom->seek_ss &= 0xF0; }
            if (cdrom->seek_ss == 0x60) { cdrom->seek_mm++; cdrom->seek_ss = 0; }
            if ((cdrom->seek_mm & 0xF) == 10) { cdrom->seek_mm += 0x10; cdrom->seek_mm &= 0xF0; }

            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = double_speed ? READ_DOUBLE_DELAY : READ_SINGLE_DELAY;
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_READN;
            cdrom->dfifo_index = 0;

            SET_BITS(ifr, IFR_INT, IFR_INT1);
            RESP_PUSH(GETSTAT_MOTOR | GETSTAT_READ);
        } break;
    }
}
void cdrom_cmd_motoron(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            cdrom->read_ongoing = 0;
            cdrom->cdda_playing = 0;

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_MOTORON;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(GETSTAT_MOTOR | GETSTAT_READ);

            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = DELAY_1MS * (double_speed ? 70 : 65);
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_MOTORON;
        } break;

        case CD_STATE_SEND_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->state = CD_STATE_RECV_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_stop(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            cdrom->read_ongoing = 0;
            cdrom->cdda_playing = 0;

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_STOP;
            cdrom->seek_ff = 0;
            cdrom->seek_ss = 0;
            cdrom->seek_mm = 0;
            cdrom->cdda_msf.m = 0;
            cdrom->cdda_msf.s = 0;
            cdrom->cdda_msf.f = 0;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(GETSTAT_MOTOR | GETSTAT_READ);

            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = DELAY_1MS * (double_speed ? 70 : 65);
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_STOP;
        } break;

        case CD_STATE_SEND_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->state = CD_STATE_RECV_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_pause(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            cdrom->read_ongoing = 0;
            cdrom->cdda_playing = 0;

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_PAUSE;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(GETSTAT_MOTOR | GETSTAT_READ);

            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = DELAY_1MS * (double_speed ? 70 : 65);
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_PAUSE;
        } break;

        case CD_STATE_SEND_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->state = CD_STATE_RECV_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_init(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_INIT;
            cdrom->read_ongoing = 0;
            cdrom->mode = 0;
            cdrom->dfifo_index = 0;
            cdrom->dfifo_full = 0;
            cdrom->pfifo_index = 0;
            cdrom->rfifo_index = 0;
            cdrom->seek_mm = 0;
            cdrom->seek_ss = 0;
            cdrom->seek_ff = 0;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(cdrom->stat);

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_INIT;
        } break;

        case CD_STATE_SEND_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(cdrom->stat);

            cdrom->state = CD_STATE_RECV_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_unmute(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlUnmute: Expected exactly 0 parameters");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_DEMUTE;
            cdrom->state = CD_STATE_SEND_RESP1;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(cdrom->stat);

            cdrom->delayed_command = CDL_NONE;
            cdrom->state = CD_STATE_RECV_CMD;
        } break;
    }
}
void cdrom_cmd_setfilter(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index != 2) {
                log_fatal("CdlSetfilter: Expected exactly 2 parameter");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->pfifo_index = 0;

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_SETFILTER;
            cdrom->state = CD_STATE_SEND_RESP1;
        } break;

        case CD_STATE_SEND_RESP1: {
            cdrom->delayed_command = CDL_NONE;
    
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(cdrom->stat);

            cdrom->state = CD_STATE_RECV_CMD;
        } break;
    }
}
void cdrom_cmd_setmode(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index != 1) {
                log_fatal("CdlSetmode: Expected exactly 1 parameter");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            int prev_speed = cdrom->mode & MODE_SPEED;

            cdrom->mode = PFIFO_POP;

            if ((cdrom->mode & MODE_SPEED) != prev_speed)
                cdrom->spin_delay = DELAY_1MS * 650;

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_SETMODE;
            cdrom->state = CD_STATE_SEND_RESP1;
        } break;

        case CD_STATE_SEND_RESP1: {
            cdrom->delayed_command = CDL_NONE;
    
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->state = CD_STATE_RECV_CMD;
        } break;
    }
}
void cdrom_cmd_getlocl(psx_cdrom_t* cdrom) { log_fatal("getlocl: Unimplemented"); exit(1); }
void cdrom_cmd_getlocp(psx_cdrom_t* cdrom) {
    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_GETLOCP;
            cdrom->state = CD_STATE_SEND_RESP1;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x00);
            RESP_PUSH(0x01);
            RESP_PUSH(0xff);

            cdrom->delayed_command = CDL_NONE;
            cdrom->state = CD_STATE_RECV_CMD;
        } break;
    }
}
void cdrom_cmd_gettn(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlGetTN: Expected exactly 0 parameters");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_GETTN;
            cdrom->state = CD_STATE_SEND_RESP1;
        } break;

        case CD_STATE_SEND_RESP1: {
            int tn;

            psx_disc_get_track_count(cdrom->disc, &tn);

            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(tn);
            RESP_PUSH(0x01);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->delayed_command = CDL_NONE;
            cdrom->state = CD_STATE_RECV_CMD;
        } break;
    }
}
void cdrom_cmd_gettd(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index != 1) {
                log_fatal("CdlGetTD: Expected exactly 0 parameters");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->gettd_track = PFIFO_POP;

            int err = psx_disc_get_track_addr(cdrom->disc, NULL, cdrom->gettd_track);

            if (err) {
                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_GETTD;
            cdrom->state = CD_STATE_SEND_RESP1;
        } break;

        case CD_STATE_SEND_RESP1: {
            msf_t td;

            psx_disc_get_track_addr(cdrom->disc, &td, cdrom->gettd_track);

            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(ITOB(td.s));
            RESP_PUSH(ITOB(td.m));
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->delayed_command = CDL_NONE;
            cdrom->state = CD_STATE_RECV_CMD;
        } break;
    }
}
void cdrom_cmd_seekl(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlSeekL: Expected exactly 0 parameters");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_SEEKL;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_SEEKL;

            msf_t msf;

            msf.m = cdrom->seek_mm;
            msf.s = cdrom->seek_ss;
            msf.f = cdrom->seek_ff;

            msf_from_bcd(&msf);

            psx_disc_seek(cdrom->disc, msf);

            cdrom->seek_pending = 0;
        } break;

        case CD_STATE_SEND_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->state = CD_STATE_RECV_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_seekp(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlSeekP: Expected exactly 0 parameters");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_SEEKP;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_SEEKP;

            msf_t msf;

            msf.m = cdrom->seek_mm;
            msf.s = cdrom->seek_ss;
            msf.f = cdrom->seek_ff;

            msf_from_bcd(&msf);

            psx_disc_seek(cdrom->disc, msf);

            cdrom->seek_pending = 0;
        } break;

        case CD_STATE_SEND_RESP2: {
            SET_BITS(ifr, IFR_INT, 2);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->state = CD_STATE_RECV_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_test(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index != 1) {
                log_fatal("CdlTest: Expected exactly 1 parameter");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            if (PFIFO_POP != 0x20) {
                log_fatal("CdlTest: Unhandled subcommand");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_INVSUBF;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->delayed_command = CDL_TEST;
            cdrom->state = CD_STATE_SEND_RESP1;
        } break;

        case CD_STATE_SEND_RESP1: {
            cdrom->delayed_command = CDL_NONE;
    
            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(0x01);
            RESP_PUSH(0x95);
            RESP_PUSH(0x13);
            RESP_PUSH(0x03);

            cdrom->state = CD_STATE_RECV_CMD;
        } break;
    }
}
void cdrom_cmd_getid(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            if (cdrom->pfifo_index) {
                log_fatal("CdlGetID: Expected exactly 0 parameters");

                cdrom->irq_delay = DELAY_1MS;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_PCOUNT;
                cdrom->error_flags = GETSTAT_ERROR;

                return;
            }

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_GETID;
        } break;

        case CD_STATE_SEND_RESP1: {
            SET_BITS(ifr, IFR_INT, 3);
            RESP_PUSH(GETSTAT_MOTOR);

            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_GETID;
        } break;

        case CD_STATE_SEND_RESP2: {
            if (cdrom->disc) {
                SET_BITS(ifr, IFR_INT, 2);

                switch (cdrom->cd_type) {
                    case CDT_LICENSED: {
                        for (int i = 0; i < GETID_RESPONSE_SIZE; i++)
                            RESP_PUSH(g_getid_licensed[GETID_RESPONSE_END - i]);
                    } break;

                    case CDT_AUDIO: {
                        for (int i = 0; i < GETID_RESPONSE_SIZE; i++)
                            RESP_PUSH(g_getid_audio[GETID_RESPONSE_END - i]);
                    } break;

                    case CDT_UNKNOWN: {
                        for (int i = 0; i < GETID_RESPONSE_SIZE; i++)
                            RESP_PUSH(g_getid_unlicensed[GETID_RESPONSE_END - i]);
                    } break;
                }
            } else {
                SET_BITS(ifr, IFR_INT, 5);

                for (int i = 0; i < GETID_RESPONSE_SIZE; i++)
                    RESP_PUSH(g_getid_no_disc[GETID_RESPONSE_END - i]);
            }

            cdrom->state = CD_STATE_RECV_CMD;
            cdrom->delayed_command = CDL_NONE;
        } break;
    }
}
void cdrom_cmd_reads(psx_cdrom_t* cdrom) {
    cdrom->delayed_command = CDL_NONE;
    cdrom->read_ongoing = 1;

    switch (cdrom->state) {
        case CD_STATE_RECV_CMD: {
            log_fatal("CdlReadS: CD_STATE_RECV_CMD");
            cdrom->irq_delay = DELAY_1MS;
            cdrom->state = CD_STATE_SEND_RESP1;
            cdrom->delayed_command = CDL_READS;
        } break;

        case CD_STATE_SEND_RESP1: {
            log_fatal("CdlReadS: CD_STATE_SEND_RESP1");

            SET_BITS(ifr, IFR_INT, IFR_INT3);
            RESP_PUSH(GETSTAT_MOTOR);

            msf_t msf;

            msf.m = cdrom->seek_mm;
            msf.s = cdrom->seek_ss;
            msf.f = cdrom->seek_ff;

            msf_from_bcd(&msf);

            int err = psx_disc_seek(cdrom->disc, msf);

            if (err) {
                log_fatal("CdlReadS: Out of bounds seek");

                cdrom->irq_delay = DELAY_1MS * 600;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_INVSUBF;
                cdrom->error_flags = GETSTAT_SEEKERROR;

                return;
            }

            psx_disc_read_sector(cdrom->disc, cdrom->dfifo);

            msf_t sector_msf;

            sector_msf.m = cdrom->dfifo[0x0c];
            sector_msf.s = cdrom->dfifo[0x0d];
            sector_msf.f = cdrom->dfifo[0x0e];

            int correct_msf = (cdrom->seek_mm == sector_msf.m) && 
                              (cdrom->seek_ss == sector_msf.s) &&
                              (cdrom->seek_ff == sector_msf.f);
            
            // Most probably audio sector:
            // Purposefully constructed audio data could
            // circumvent this detection code, but it will work
            // for most intents and purposes 
            if (!correct_msf) {
                log_fatal("CdlReadS: Audio read");

                cdrom->irq_delay = DELAY_1MS * 600;
                cdrom->delayed_command = CDL_ERROR;
                cdrom->state = CD_STATE_ERROR;
                cdrom->error = ERR_SEEK;
                cdrom->error_flags = GETSTAT_SEEKERROR;

                return;
            }
            
            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = double_speed ? READ_DOUBLE_DELAY : READ_SINGLE_DELAY;
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_READS;

            if (cdrom->spin_delay) {
                cdrom->irq_delay += cdrom->spin_delay;
                cdrom->spin_delay = 0;
            }
        } break;

        case CD_STATE_SEND_RESP2: {
            log_fatal("CdlReadS: CD_STATE_SEND_RESP2");

            msf_t msf;

            msf.m = cdrom->seek_mm;
            msf.s = cdrom->seek_ss;
            msf.f = cdrom->seek_ff;

            msf_from_bcd(&msf);

            psx_disc_seek(cdrom->disc, msf);
            psx_disc_read_sector(cdrom->disc, cdrom->dfifo);

            if (cdrom->mode & MODE_XA_ADPCM) {
                cdrom->state = CD_STATE_RECV_CMD;
                cdrom->delayed_command = CDL_NONE;

                return;
            }

            if (cdrom->dfifo[0x12] & 0x20) {
                log_fatal("Unimplemented XA Form2 Sector");

                // exit(1);
            }

            cdrom->seek_ff++;

            if ((cdrom->seek_ff & 0xF) == 10) { cdrom->seek_ff += 0x10; cdrom->seek_ff &= 0xF0; }
            if (cdrom->seek_ff == 0x75) { cdrom->seek_ss++; cdrom->seek_ff = 0; }
            if ((cdrom->seek_ss & 0xF) == 10) { cdrom->seek_ss += 0x10; cdrom->seek_ss &= 0xF0; }
            if (cdrom->seek_ss == 0x60) { cdrom->seek_mm++; cdrom->seek_ss = 0; }
            if ((cdrom->seek_mm & 0xF) == 10) { cdrom->seek_mm += 0x10; cdrom->seek_mm &= 0xF0; }

            int double_speed = cdrom->mode & MODE_SPEED;

            cdrom->irq_delay = double_speed ? READ_DOUBLE_DELAY : READ_SINGLE_DELAY;
            cdrom->state = CD_STATE_SEND_RESP2;
            cdrom->delayed_command = CDL_READS;
            cdrom->dfifo_index = 0;

            SET_BITS(ifr, IFR_INT, IFR_INT1);
            RESP_PUSH(GETSTAT_MOTOR | GETSTAT_READ);
        } break;
    }
}
void cdrom_cmd_readtoc(psx_cdrom_t* cdrom) { log_fatal("readtoc: Unimplemented"); exit(1); }

typedef void (*cdrom_cmd_t)(psx_cdrom_t*);

const char* g_psx_cdrom_command_names[] = {
    "CdlUnimplemented",
    "CdlGetstat",
    "CdlSetloc",
    "CdlPlay",
    "CdlUnimplemented",
    "CdlUnimplemented",
    "CdlReadn",
    "CdlMotoron",
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
    cdrom_cmd_play,
    cdrom_cmd_unimplemented,
    cdrom_cmd_unimplemented,
    cdrom_cmd_readn,
    cdrom_cmd_motoron,
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

    // Actually an unimplemented command, we use this
    // index for CD error handling
    cdrom_cmd_error
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

    uint32_t sector_size = sector_size_bit ? 0x924 : 0x800;
    uint32_t offset = sector_size_bit ? 12 : 24;

    if (cdrom->dfifo_index < sector_size) {
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
    log_set_quiet(0);
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
    log_set_quiet(1);

    cdrom->command = value;
    cdrom->state = CD_STATE_RECV_CMD;

    g_psx_cdrom_command_table[value](cdrom);
}

void cdrom_write_pfifo(psx_cdrom_t* cdrom, uint8_t value) {
    cdrom->pfifo[(cdrom->pfifo_index++) & 0xf] = value;

    SET_BITS(status, STAT_PRMWRDY_MASK, (cdrom->pfifo_index & 0x10) ? 0x0 : 0xff);

    cdrom->pfifo_index &= 0x1f;
}

void cdrom_write_req(psx_cdrom_t* cdrom, uint8_t value) {
    if (value & REQ_BFRD) {
        SET_BITS(status, STAT_DRQSTS_MASK, STAT_DRQSTS_MASK);

        cdrom->dfifo_full = 1;
        cdrom->dfifo_index = 0;
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
    cdrom->ifr &= ~(value & 0x1f);

    // if (value & 0x7) {
    //     log_set_quiet(0);
    //     log_fatal("Acknowledge %02x", value & 0x7);
    //     log_set_quiet(1);
    // }

    // Clear Parameter FIFO
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
    cdrom->status = STAT_PRMEMPT_MASK | STAT_PRMWRDY_MASK | STAT_RSLRRDY_MASK;
    cdrom->dfifo = malloc(CD_SECTOR_SIZE);
    cdrom->cdda_buf = malloc(CD_SECTOR_SIZE);
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

void psx_cdrom_update(psx_cdrom_t* cdrom, int cyc) {
    if (cdrom->irq_delay) {
        cdrom->irq_delay -= cyc;

        if (cdrom->irq_delay <= 0) {
            psx_ic_irq(cdrom->ic, IC_CDROM);

            cdrom->irq_delay = 0;

            if (cdrom->delayed_command) {
                // log_set_quiet(0);
                // log_fatal("%s(%02x) (Delayed)",
                //     g_psx_cdrom_command_names[cdrom->delayed_command],
                //     cdrom->delayed_command
                // );
                // log_set_quiet(1);
                g_psx_cdrom_command_table[cdrom->delayed_command](cdrom);
            }

            // log_set_quiet(0);
            // log_fatal("CDROM INT%u", cdrom->ifr & 0x7);
            // log_set_quiet(1);
        }
    }
}

const char* g_psx_cdrom_extensions[] = {
    "cue",
    "bin",
    0
};

enum {
    CD_EXT_CUE,
    CD_EXT_BIN,
    CD_EXT_UNSUPPORTED
};

int cdrom_get_extension(const char* path) {
    const char* ptr = &path[strlen(path) - 1];
    int i = 0;

    while ((*ptr != '.') && (ptr != path))
        ptr--;
    
    if (ptr == path)
        return CD_EXT_UNSUPPORTED;

    while (g_psx_cdrom_extensions[i]) {
        if (!strcmp(ptr + 1, g_psx_cdrom_extensions[i]))
            return i;
        
        ++i;
    }

    return CD_EXT_UNSUPPORTED;
}

void cdrom_check_cd_type(psx_cdrom_t* cdrom) {
    char buf[CD_SECTOR_SIZE];

    // Seek to Primary Volume Descriptor
    msf_t pvd = { 0, 2, 16 };

    // If the disc is smaller than 16 sectors
    // then it can't be a PlayStation game.
    // Audio discs should also have ISO volume
    // descriptors, so it's probably something else
    // entirely.
    if (psx_disc_seek(cdrom->disc, pvd)) {
        cdrom->cd_type = CDT_UNKNOWN;

        return;
    }

    psx_disc_read_sector(cdrom->disc, buf);

    // Check for the "PLAYSTATION" string at PVD offset 20h

    // Patch 20 byte so comparison is done correctly
    buf[0x2b] = 0;

    if (strncmp(&buf[0x20], "PLAYSTATION", 12)) {
        cdrom->cd_type = CDT_AUDIO;

        return;
    }

    cdrom->cd_type = CDT_LICENSED;
}

void psx_cdrom_open(psx_cdrom_t* cdrom, const char* path) {
    cdrom->disc = psx_disc_create();

    int ext = cdrom_get_extension(path);
    int error = 0;

    switch (ext) {
        case CD_EXT_CUE: {
            psxd_cue_t* cue = psxd_cue_create();

            psxd_cue_init_disc(cue, cdrom->disc);
            psxd_cue_init(cue);
            error = psxd_cue_load(cue, path);

            if (error)
                break;

            cdrom_check_cd_type(cdrom);
        } break;

        case CD_EXT_BIN: {
            psxd_bin_t* bin = psxd_bin_create();

            psxd_bin_init_disc(bin, cdrom->disc);
            psxd_bin_init(bin);

            error = psxd_bin_load(bin, path);

            if (error)
                break;

            cdrom_check_cd_type(cdrom);
        } break;

        case CD_EXT_UNSUPPORTED: {
            log_fatal("Unsupported disc format");

            cdrom->cd_type = CDT_UNKNOWN;
        } break;
    }

    if (error) {
        log_fatal("Error loading file \'%s\'", path);

        exit(1);
    }
}

void psx_cdrom_get_cdda_samples(psx_cdrom_t* cdrom, void* buf, int size, psx_spu_t* spu) {
    if (!cdrom->cdda_playing) {
        memset(buf, 0, size);
    
        return;
    }

    if (!cdrom->disc)
        return;

    // Convert seek to I
    msf_t msf = cdrom->cdda_msf;
    
    msf_from_bcd(&msf);

    // Seek to that address and read sector
    if (psx_disc_seek(cdrom->disc, msf)) {
        cdrom->cdda_playing = 0;
    }

    psx_disc_read_sector(cdrom->disc, cdrom->cdda_buf);

    ++cdrom->cdda_sectors_played;

    // Increment sector
    msf_add_f(&msf, 1);
    msf_to_bcd(&msf);
    
    // Assign to CDDA MSF
    cdrom->cdda_msf = msf;

    memcpy(buf, cdrom->cdda_buf, size);

    psx_spu_update_cdda_buffer(spu, cdrom->cdda_buf);

    // Handle report IRQ
    if (cdrom->cdda_sectors_played == CD_SECTORS_PS) {
        if (cdrom->mode & MODE_REPORT) {
            SET_BITS(ifr, IFR_INT, 1);

            msf_t track, current = cdrom->cdda_msf;

            msf_from_bcd(&current);

            psx_disc_get_track_addr(cdrom->disc, &track, cdrom->cdda_track);

            unsigned int track_s = (track.m * 60) + track.s;
            unsigned int current_s = (current.m * 60) + current.s;
            unsigned int diff = current_s - track_s;

            current.s = diff;
            current.m = 0;

            msf_adjust(&current);
            msf_to_bcd(&current);

            RESP_PUSH(0);
            RESP_PUSH(0);
            RESP_PUSH(cdrom->cdda_msf.f);
            RESP_PUSH(current.s | 0x80);
            RESP_PUSH(current.m);
            RESP_PUSH(0);
            RESP_PUSH(cdrom->cdda_track);
            RESP_PUSH(GETSTAT_PLAY);

            psx_ic_irq(cdrom->ic, IC_CDROM);
        }

        cdrom->cdda_sectors_played = 0;
    }
}

void psx_cdrom_destroy(psx_cdrom_t* cdrom) {
    if (cdrom->disc)
        psx_disc_destroy(cdrom->disc);

    free(cdrom);
}