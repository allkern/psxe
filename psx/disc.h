/*
    This file is part of the PSXE Emulator Project

    Disc Reader API
*/

#ifndef DISC_H
#define DISC_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    uint8_t m;
    uint8_t s;
    uint8_t f;
} msf_t;

typedef int (*disc_seek_t)(void*, msf_t);
typedef int (*disc_read_sector_t)(void*, void*);
typedef int (*disc_get_track_t)(void*, msf_t*, int);

typedef struct {
    void* udata;

    disc_seek_t seek_func;
    disc_read_sector_t read_sector_func;
    disc_get_track_t get_track_func;
} psx_disc_t;

psx_disc_t* psx_disc_create();
int psx_disc_seek(psx_disc_t*, msf_t);
int psx_disc_read_sector(psx_disc_t*, void*);
int psx_disc_get_track(psx_disc_t*, msf_t*, int);

#endif