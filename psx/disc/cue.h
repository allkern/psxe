/*
    This file is part of the PSXE Emulator Project

    CUE Parser + Loader
*/

#ifndef CUE_H
#define CUE_H

#include "../disc.h"

enum {
    PE_EXPECTED_KEYWORD = 1,
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

static const char* g_psxd_cue_tokens[] = {
    "4CH",
    "AIFF",
    "AUDIO",
    "BINARY",
    "CATALOG",
    "CDG",
    "CDI_2336",
    "CDI_2352",
    "CDTEXTFILE",
    "DCP",
    "FILE",
    "FLAGS",
    "INDEX",
    "ISRC",
    "MODE1_2048",
    "MODE1_2352",
    "MODE2_2336",
    "MODE2_2352",
    "MOTOROLA",
    "MP3",
    "PERFORMER",
    "POSTGAP",
    "PRE",
    "PREGAP",
    "REM",
    "SCMS",
    "SONGWRITER",
    "TITLE",
    "TRACK",
    "WAVE"
};

typedef struct {

} psxd_cue_t;

psxd_cue_t* psxd_cue_create();
void psxd_cue_init(psxd_cue_t*);
void psxd_cue_load(psxd_cue_t*, const char*);
void psxd_cue_init_disc(psxd_cue_t*, psx_disc_t*);
void psxd_cue_destroy(psxd_cue_t*);

#endif