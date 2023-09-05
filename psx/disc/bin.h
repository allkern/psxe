/*
    This file is part of the PSXE Emulator Project

    BIN Loader
*/

#ifndef BIN_H
#define BIN_H

#include "../disc.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char* buf;
    uint32_t buf_size;
    uint32_t seek_offset;
    msf_t end;
} psxd_bin_t;

psxd_bin_t* psxd_bin_create();
void psxd_bin_init(psxd_bin_t*);
int psxd_bin_load(psxd_bin_t*, const char*);
void psxd_bin_init_disc(psxd_bin_t*, psx_disc_t*);
void psxd_bin_destroy(psxd_bin_t*);

#endif