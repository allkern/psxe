#ifndef CDROM_H
#define CDROM_H

#include <stdint.h>
#include <stdio.h>

#include "ic.h"
#include "../disc.h"
#include "../disc/cue.h"
#include "../disc/bin.h"
#include "../msf.h"
#include "spu.h"

// #define DELAY_1MS (0xc4e1)
// #define READ_SINGLE_DELAY (0x6e1cd)
// #define READ_DOUBLE_DELAY (0x36cd2)
#define DELAY_1MS (PSX_CPU_CPS / 1000)
#define READ_SINGLE_DELAY (PSX_CPU_CPS / 75)
#define READ_DOUBLE_DELAY (PSX_CPU_CPS / (2 * 75))

#define PSX_CDROM_BEGIN 0x1f801800
#define PSX_CDROM_SIZE  0x4
#define PSX_CDROM_END   0x1f801803

enum {
    CD_STATE_RECV_CMD,
    CD_STATE_SEND_RESP1,
    CD_STATE_SEND_RESP2,
    CD_STATE_ERROR
};

#define CDL_NONE        0x00
#define CDL_GETSTAT     0x01
#define CDL_SETLOC      0x02
#define CDL_PLAY        0x03
#define CDL_FORWARD     0x04
#define CDL_BACKWARD    0x05
#define CDL_READN       0x06
#define CDL_MOTORON     0x07
#define CDL_STOP        0x08
#define CDL_PAUSE       0x09
#define CDL_INIT        0x0a
#define CDL_MUTE        0x0b
#define CDL_DEMUTE      0x0c
#define CDL_SETFILTER   0x0d
#define CDL_SETMODE     0x0e
#define CDL_GETPARAM    0x0f
#define CDL_GETLOCL     0x10
#define CDL_GETLOCP     0x11
#define CDL_SETSESSION  0x12
#define CDL_GETTN       0x13
#define CDL_GETTD       0x14
#define CDL_SEEKL       0x15
#define CDL_SEEKP       0x16
#define CDL_TEST        0x19
#define CDL_GETID       0x1a
#define CDL_READS       0x1b
#define CDL_RESET       0x1c
#define CDL_GETQ        0x1d
#define CDL_READTOC     0x1e
#define CDL_VIDEOCD     0x1f
#define CDL_ERROR       0x20

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

#define GETSTAT_ERROR      0x01
#define GETSTAT_MOTOR      0x02
#define GETSTAT_SEEKERROR  0x04
#define GETSTAT_IDERROR    0x08
#define GETSTAT_TRAYOPEN   0x10
#define GETSTAT_READ       0x20
#define GETSTAT_SEEK       0x40
#define GETSTAT_PLAY       0x80

/*
  7   Speed       (0=Normal speed, 1=Double speed)
  6   XA-ADPCM    (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
  5   Sector Size (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
  4   Ignore Bit  (0=Normal, 1=Ignore Sector Size and Setloc position)
  3   XA-Filter   (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
  2   Report      (0=Off, 1=Enable Report-Interrupts for Audio Play)
  1   AutoPause   (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
  0   CDDA        (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)
*/

#define MODE_CDDA           0x01
#define MODE_AUTOPAUSE      0x02
#define MODE_REPORT         0x04
#define MODE_XA_FILTER      0x08
#define MODE_IGNORE         0x10
#define MODE_SECTOR_SIZE    0x20
#define MODE_XA_ADPCM       0x40
#define MODE_SPEED          0x80

/*
  0-4 0    Not used (should be zero)
  5   SMEN Want Command Start Interrupt on Next Command (0=No change, 1=Yes)
  6   BFWR ...
  7   BFRD Want Data         (0=No/Reset Data Fifo, 1=Yes/Load Data Fifo)
*/

#define REQ_SMEN 0x20
#define REQ_BFWR 0x40
#define REQ_BFRD 0x80

/*
  ___These values appear in the FIRST response; with stat.bit0 set___
  10h - Invalid Sub_function (for command 19h), or invalid parameter value
  20h - Wrong number of parameters
  40h - Invalid command
  80h - Cannot respond yet (eg. required info was not yet read from disk yet)
           (namely, TOC not-yet-read or so)
           (also appears if no disk inserted at all)
  ___These values appear in the SECOND response; with stat.bit2 set___
  04h - Seek failed (when trying to use SeekL on Audio CDs)
  ___These values appear even if no command was sent; with stat.bit2 set___
  08h - Drive door became opened
*/

#define ERR_INVSUBF 0x10
#define ERR_PCOUNT  0x20
#define ERR_INVALID 0x40
#define ERR_BUSY    0x80
#define ERR_SEEK    0x04
#define ERR_LIDOPEN 0x08

enum {
    CDT_LICENSED,
    CDT_AUDIO,
    CDT_UNKNOWN
};

typedef struct {
    uint32_t bus_delay;
    uint32_t io_base, io_size;

    psx_ic_t* ic;

    uint8_t status;
    uint8_t ifr;
    uint8_t ier;

    uint8_t pfifo[16];
    uint8_t rfifo[16];
    int pfifo_index;
    int rfifo_index;

    uint8_t* read_buf;
    uint8_t* dfifo;
    int dfifo_index;
    int dfifo_full;

    // GetStat bits
    uint8_t stat;

    // API
    int tray_open;

    // Setloc
    msf_t seek_msf;
    uint32_t seek_offset;
    int seek_pending;

    // Setmode
    uint8_t mode;

    int irq_delay;
    int irq_disable;
    uint8_t command;
    uint8_t delayed_command;
    uint8_t int_number;
    int state;
    int delayed_response;
    int spin_delay;
    uint8_t error;
    uint8_t error_flags;
    int ongoing_read_command;
    int gettd_track;

    // CDDA
    uint8_t* cdda_buf;
    int cdda_sector_offset;
    msf_t cdda_msf;
    int cdda_playing;
    int cdda_sectors_played;
    int cdda_track;

    // XA-ADPCM
    uint8_t* xa_sector_buf;
    msf_t xa_msf;
    int xa_playing;
    uint8_t xa_file;
    uint8_t xa_channel;
    uint8_t xa_coding;
    int16_t* xa_left_buf;
    int16_t* xa_right_buf;
    int16_t* xa_mono_buf;
    int16_t* xa_decoded_buf;
    int16_t* xa_left_ring_buf;
    int16_t* xa_right_ring_buf;
    uint32_t xa_sample_idx;
    int xa_remaining_samples;
    uint32_t xa_step;
    uint32_t xa_ringbuf_pos;
    int16_t* xa_left_resample_buf;
    int16_t* xa_right_resample_buf;
    int16_t* xa_mono_resample_buf;
    int16_t* xa_upsample_buf;
    int16_t xa_last_left_sample;
    int16_t xa_last_right_sample;
    int16_t xa_last_mono_sample;

    const char* path;
    psx_disc_t* disc;

    int cd_type;
} psx_cdrom_t;

psx_cdrom_t* psx_cdrom_create();
void psx_cdrom_init(psx_cdrom_t*, psx_ic_t*);
uint32_t psx_cdrom_read32(psx_cdrom_t*, uint32_t);
uint16_t psx_cdrom_read16(psx_cdrom_t*, uint32_t);
uint8_t psx_cdrom_read8(psx_cdrom_t*, uint32_t);
void psx_cdrom_write32(psx_cdrom_t*, uint32_t, uint32_t);
void psx_cdrom_write16(psx_cdrom_t*, uint32_t, uint16_t);
void psx_cdrom_write8(psx_cdrom_t*, uint32_t, uint8_t);
void psx_cdrom_update(psx_cdrom_t*, int);
void psx_cdrom_destroy(psx_cdrom_t*);
void psx_cdrom_open(psx_cdrom_t*, const char*);
void psx_cdrom_get_cdda_samples(psx_cdrom_t*, void*, int, psx_spu_t*);

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

void cdrom_cmd_unimplemented(psx_cdrom_t*);
void cdrom_cmd_getstat(psx_cdrom_t*);
void cdrom_cmd_setloc(psx_cdrom_t*);
void cdrom_cmd_play(psx_cdrom_t*);
void cdrom_cmd_readn(psx_cdrom_t*);
void cdrom_cmd_motoron(psx_cdrom_t*);
void cdrom_cmd_stop(psx_cdrom_t*);
void cdrom_cmd_pause(psx_cdrom_t*);
void cdrom_cmd_init(psx_cdrom_t*);
void cdrom_cmd_unmute(psx_cdrom_t*);
void cdrom_cmd_setfilter(psx_cdrom_t*);
void cdrom_cmd_setmode(psx_cdrom_t*);
void cdrom_cmd_getparam(psx_cdrom_t*);
void cdrom_cmd_getlocl(psx_cdrom_t*);
void cdrom_cmd_getlocp(psx_cdrom_t*);
void cdrom_cmd_setsession(psx_cdrom_t*);
void cdrom_cmd_gettn(psx_cdrom_t*);
void cdrom_cmd_gettd(psx_cdrom_t*);
void cdrom_cmd_seekl(psx_cdrom_t*);
void cdrom_cmd_seekp(psx_cdrom_t*);
void cdrom_cmd_test(psx_cdrom_t*);
void cdrom_cmd_getid(psx_cdrom_t*);
void cdrom_cmd_reads(psx_cdrom_t*);
void cdrom_cmd_readtoc(psx_cdrom_t*);
void cdrom_cmd_videocd(psx_cdrom_t*);

#endif