/*
    This file is part of the PSXE Emulator Project

    BIN Loader
*/

#include "bin.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

psxd_bin_t* psxd_bin_create() {
    return (psxd_bin_t*)malloc(sizeof(psxd_bin_t));
}

void psxd_bin_init(psxd_bin_t* bin) {
    memset(bin, 0, sizeof(psxd_bin_t));
}

int psxd_bin_load(psxd_bin_t* bin, const char* path) {
    log_fatal("Loading CD image...");

    FILE* file = fopen(path, "rb");

    if (ferror(file) || !file) {
        fclose(file);

        return 1;
    }

    fseek(file, 0, SEEK_END);

    bin->buf_size = ftell(file);
    
    fseek(file, 0, SEEK_SET);

    bin->buf = malloc(bin->buf_size);

    if (!fread(bin->buf, 1, bin->buf_size, file)) {
        perror("Error reading BIN CD image file data");

        exit(1);
    }

    msf_from_address(&bin->end, bin->buf_size);

    fclose(file);

    log_fatal("Loaded BIN image, size=%08x, end=%02u:%02u:%02u",
        bin->buf_size,
        bin->end.m,
        bin->end.s,
        bin->end.f
    );

    return 0;
}

int psxd_bin_seek(void* udata, msf_t msf) {
    psxd_bin_t* bin = udata;

    msf.s -= 2;

    bin->seek_offset = msf_to_address(msf);

    log_fatal("BIN seek to %02u:%02u:%02u (%08x < %08x)", msf.m, msf.s, msf.f, bin->seek_offset, bin->buf_size);

    if (bin->seek_offset >= bin->buf_size)
        return DISC_ERR_ADDR_OUT_OF_BOUNDS;

    return 0;
}

int psxd_bin_read_sector(void* udata, void* buf) {
    psxd_bin_t* bin = udata;

    log_fatal("BIN reading sector at offset %08x", bin->seek_offset);

    memcpy(buf, bin->buf + bin->seek_offset, CD_SECTOR_SIZE);

    return 0;
}

int psxd_bin_get_track_addr(void* udata, msf_t* msf, int track) {
    if (track > 1)
        return DISC_ERR_TRACK_OUT_OF_BOUNDS;

    msf->m = 0;
    msf->s = 2;

    return 0;
}

int psxd_bin_get_track_count(void* udata, int* count) {
    *count = 1;

    return 0;
}

void psxd_bin_init_disc(psxd_bin_t* bin, psx_disc_t* disc) {
    disc->udata = bin;
    disc->seek_func = psxd_bin_seek;
    disc->read_sector_func = psxd_bin_read_sector;
    disc->get_track_addr_func = psxd_bin_get_track_addr;
    disc->get_track_count_func = psxd_bin_get_track_count;
    disc->destroy_func = (disc_destroy_t)psxd_bin_destroy;
}

void psxd_bin_destroy(psxd_bin_t* bin) {
    free(bin->buf);
    free(bin);
}