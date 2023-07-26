/*
    This file is part of the PSXE Emulator Project

    Disc Reader API
*/

#include "disc.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define CD_SECTOR_SIZE 0x930
#define CD_SECTORS_PER_SECOND 75

uint32_t disc_get_addr(msf_t msf) {
    uint32_t sectors = (((msf.m * 60) + msf.s) * CD_SECTORS_PER_SECOND) + msf.f;

    return sectors * CD_SECTOR_SIZE;
}

psx_disc_t* psx_disc_create() {
    return (psx_disc_t*)malloc(sizeof(psx_disc_t));
}

int psx_disc_seek(psx_disc_t* disc, msf_t msf) {
    return disc->seek_func(msf);
}

int psx_disc_read_sector(psx_disc_t* disc, void* buf) {
    return disc->read_sector_func(buf);
}

int psx_disc_get_track(psx_disc_t* disc, msf_t* msf, int track) {
    return disc->get_track_func(msf, track);
}