/*
    This file is part of the PSXE Emulator Project

    CUE Parser + Loader
*/

#ifndef CUE_H
#define CUE_H

#define CUE_BUF_SIZE 256

#include "../disc.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

enum {
    PE_EXPECTED_KEYWORD = 1,
    PE_EXPECTED_STRING,
    PE_EXPECTED_NUMBER,
    PE_EXPECTED_COLON,
    PE_NON_SEQUENTIAL_TRACKS,
    PE_UNEXPECTED_TOKEN
};

enum {
    CUE_NONE,
    CUE_4CH,
    CUE_AIFF,
    CUE_AUDIO,
    CUE_BINARY,
    CUE_CATALOG,
    CUE_CDG,
    CUE_CDI_2336,
    CUE_CDI_2352,
    CUE_CDTEXTFILE,
    CUE_DCP,
    CUE_FILE,
    CUE_FLAGS,
    CUE_INDEX,
    CUE_ISRC,
    CUE_MODE1_2048,
    CUE_MODE1_2352,
    CUE_MODE2_2336,
    CUE_MODE2_2352,
    CUE_MOTOROLA,
    CUE_MP3,
    CUE_PERFORMER,
    CUE_POSTGAP,
    CUE_PRE,
    CUE_PREGAP,
    CUE_REM,
    CUE_SCMS,
    CUE_SONGWRITER,
    CUE_TITLE,
    CUE_TRACK,
    CUE_WAVE
};

typedef struct {
    char* filename;
    int type;
    void* buf;
    msf_t index[2];
    msf_t disc_offset;
    size_t size;
} cue_track_t;

typedef struct {
    int preload;
    char* buf;
    char* ptr;
    char c;
    int error;
    FILE* file;
    int num_tracks;
    cue_track_t** track;
    char* current_file;
    uint32_t seek_offset;
    msf_t end;
} psxd_cue_t;

psxd_cue_t* psxd_cue_create();
void psxd_cue_init(psxd_cue_t*, int);
void psxd_cue_parse(psxd_cue_t*, const char*);
void psxd_cue_load(psxd_cue_t*);
void psxd_cue_init_disc(psxd_cue_t*, psx_disc_t*);
void psxd_cue_destroy(psxd_cue_t*);

#endif