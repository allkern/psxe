/*
    This file is part of the PSXE Emulator Project

    Disc Reader API
*/

#ifndef DISC_H
#define DISC_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "log.h"
#include "msf.h"

enum {
    DISC_ERR_TRACK_OUT_OF_BOUNDS = 1,
    DISC_ERR_ADDR_OUT_OF_BOUNDS
};

typedef int (*disc_seek_t)(void*, msf_t);
typedef int (*disc_read_sector_t)(void*, void*);
typedef int (*disc_get_track_addr_t)(void*, msf_t*, int);
typedef int (*disc_get_track_count_t)(void*, int*);
typedef void (*disc_destroy_t)(void*);

typedef struct {
    void* udata;

    disc_seek_t seek_func;
    disc_read_sector_t read_sector_func;
    disc_get_track_addr_t get_track_addr_func;
    disc_get_track_count_t get_track_count_func;
    disc_destroy_t destroy_func;
} psx_disc_t;

psx_disc_t* psx_disc_create(void);
int psx_disc_seek(psx_disc_t*, msf_t);
int psx_disc_read_sector(psx_disc_t*, void*);
int psx_disc_get_track_addr(psx_disc_t*, msf_t*, int);
int psx_disc_get_track_count(psx_disc_t*, int*);
void psx_disc_destroy(psx_disc_t*);

#endif