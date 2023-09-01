/*
    This file is part of the PSXE Emulator Project

    CUE Parser + Loader
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "cue.h"
#include "../disc.h"
#include "../log.h"

#define CUE_SECTOR_SIZE 0x930
#define CUE_SECTORS_PER_SECOND 75

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
    "WAVE",
    0
};

#define EXPECT_KEYWORD(kw) \
    if (cue_parse_keyword(cue)) \
        return cue->error; \
    if (cue_get_keyword(cue) != kw) \
        ERROR_OUT(PE_UNEXPECTED_TOKEN);

void cue_add_track(psxd_cue_t* cue) {
    ++cue->num_tracks;

    cue->track = realloc(cue->track, cue->num_tracks * sizeof(cue_track_t*));
    cue->track[cue->num_tracks - 1] = malloc(sizeof(cue_track_t));

    memset(cue->track[cue->num_tracks - 1], 0, sizeof(cue_track_t));
}

void* cue_alloc_block(void* buf, size_t* block_size, size_t ext) {
    *block_size += ext;

    if (!buf)
        return malloc(*block_size);
    
    return realloc(buf, *block_size);
}

void cue_ignore_whitespace(psxd_cue_t* cue) {
    while (isspace(cue->c))
        cue->c = fgetc(cue->file);
}

int cue_get_keyword(psxd_cue_t* cue) {
    int i = 0;

    const char* token = g_psxd_cue_tokens[i];

    while (token) {
        if (strcmp(token, cue->buf) == 1) {
            return i;
        } else {
            token = g_psxd_cue_tokens[++i];
        }
    }

    return -1;
}

#define ERROR_OUT(err) \
    { cue->error = err; return err; }

int cue_parse_keyword(psxd_cue_t* cue) {
    if (!isalpha(cue->c))
        ERROR_OUT(PE_EXPECTED_KEYWORD);
    
    while (isalnum(cue->c) || (cue->c == '/')) {
        *cue->ptr++ = cue->c;

        cue->c = fgetc(cue->file);
    }

    *cue->ptr = 0;

    cue->ptr = cue->buf;

    cue_ignore_whitespace(cue);

    return 0;
}

int cue_parse_string(psxd_cue_t* cue) {
    if (cue->c != '\"')
        ERROR_OUT(PE_EXPECTED_STRING);

    cue->c = fgetc(cue->file);

    while (cue->c != '\"') {
        *cue->ptr++ = cue->c;

        cue->c = fgetc(cue->file);
    }

    *cue->ptr = 0;

    cue->c = fgetc(cue->file);

    cue->ptr = cue->buf;

    cue_ignore_whitespace(cue);

    return 0;
}

int cue_parse_number(psxd_cue_t* cue) {
    if (!isdigit(cue->c))
        ERROR_OUT(PE_EXPECTED_NUMBER);
    
    while (isdigit(cue->c)) {
        *cue->ptr++ = cue->c;

        cue->c = fgetc(cue->file);
    }

    *cue->ptr = 0;

    cue->ptr = cue->buf;

    cue_ignore_whitespace(cue);

    return 0;
}

int cue_parse_msf(psxd_cue_t* cue, msf_t* msf) {
    if (cue_parse_number(cue))
        return cue->error;
    
    if (cue->c != ':')
        ERROR_OUT(PE_EXPECTED_COLON);
    
    cue->c = fgetc(cue->file);

    msf->m = atoi(cue->buf);

    if (cue_parse_number(cue))
        return cue->error;

    if (cue->c != ':')
        ERROR_OUT(PE_EXPECTED_COLON);
    
    cue->c = fgetc(cue->file);
    
    msf->s = atoi(cue->buf);

    if (cue_parse_number(cue))
        return cue->error;
    
    msf->f = atoi(cue->buf);

    return 0;
}

int cue_parse(psxd_cue_t* cue, FILE* file) {
    cue->file = file;
    cue->c = fgetc(file);

    void* filebuf;
    size_t filesz;
    msf_t msf;

    EXPECT_KEYWORD(CUE_FILE);

    parse_file:

    if (cue_parse_string(cue))
        return cue->error;

    strcpy(cue->current_file, cue->buf);

    EXPECT_KEYWORD(CUE_BINARY);
    EXPECT_KEYWORD(CUE_TRACK);

    if (cue_parse_number(cue))
        return cue->error;
    
    int track = atoi(cue->buf) - 1;
    
    if (track != cue->num_tracks)
        ERROR_OUT(PE_NON_SEQUENTIAL_TRACKS);
    
    cue_add_track(cue);

    cue->track[track]->filename = malloc(strlen(cue->current_file));

    // Copy current file to track filename
    strcpy(cue->track[track]->filename, cue->current_file);

    if (cue_parse_keyword(cue))
        return cue->error;

    cue->track[track]->type = cue_get_keyword(cue);

    // Expecting at least 1 index
    EXPECT_KEYWORD(CUE_INDEX);

    parse_index:

    if (cue_parse_number(cue))
        return cue->error;
    
    int index = atoi(cue->buf);

    if (cue_parse_msf(cue, &msf))
        return cue->error;

    cue->track[track]->index[index] = msf;

    if (feof(cue->file)) {
        fclose(file);

        return 0;
    }

    if (cue_parse_keyword(cue))
        return cue->error;
    
    switch (cue_get_keyword(cue)) {
        case CUE_INDEX:
            goto parse_index;

        case CUE_FILE:
            goto parse_file;

        default:
            ERROR_OUT(PE_UNEXPECTED_TOKEN);
    }
}

psxd_cue_t* psxd_cue_create() {
    return (psxd_cue_t*)malloc(sizeof(psxd_cue_t));
}

void psxd_cue_init(psxd_cue_t* cue, int preload) {
    memset(cue, 0, sizeof(psxd_cue_t));

    cue->buf = malloc(CUE_BUF_SIZE);
    cue->ptr = cue->buf;
    cue->preload = preload;
    cue->current_file = malloc(CUE_BUF_SIZE);
}

void psxd_cue_parse(psxd_cue_t* cue, const char* path) {
    FILE* file = fopen(path, "rb");

    cue_parse(cue, file);

    if (cue->error) {
        log_fatal("CUE error %u");

        exit(1);
    }
}

// To-do: Rework CUE loader
//        I want to put every track one after the other
//        on a big buffer for absolute MSF addressing.
//        We should also save metadata about the tracks in
//        the CUE struct.

void psxd_cue_load(psxd_cue_t* cue) {
    size_t offset = 0;

    void* buf = NULL;

    for (int i = 0; i < cue->num_tracks; i++) {
        cue_track_t* track = cue->track[i];

        FILE* file = fopen(track->filename, "rb");

        // Get track size
        fseek(file, 0, SEEK_END);

        track->size = ftell(file);

        // Calculate track MS(F)
        track->disc_offset.f = offset / CUE_SECTOR_SIZE;
        track->disc_offset.s = track->disc_offset.f / CUE_SECTORS_PER_SECOND;
        track->disc_offset.m = track->disc_offset.s / 60;
        track->disc_offset.s -= track->disc_offset.m * 60;

        buf = cue_alloc_block(buf, &offset, track->size);

        fseek(file, 0, SEEK_SET);
        fread((uint8_t*)buf + (offset - track->size), 1, track->size, file);

        fclose(file);
    }

    cue->end.f = offset / CUE_SECTOR_SIZE;
    cue->end.s = cue->end.f / CUE_SECTORS_PER_SECOND;
    cue->end.m = cue->end.s / 60;
    cue->end.s -= cue->end.m * 60;
    cue->end.f -= (cue->end.m * 60) + (cue->end.s * CUE_SECTORS_PER_SECOND);
}

int psxd_cue_seek(void* udata, msf_t msf) {
    psxd_cue_t* cue = udata;

    // To-do: Check for OOB seeks

    uint32_t sectors = (((msf.m * 60) + msf.s) * CUE_SECTORS_PER_SECOND) + msf.f;

    cue->seek_offset = sectors * CUE_SECTOR_SIZE;

    return 0;
}

int psxd_cue_read_sector(void* udata, void* buf) {
    psxd_cue_t* cue = udata;

    memcpy(buf, &cue->buf[cue->seek_offset], CUE_SECTOR_SIZE);

    return 0;
}

int psxd_cue_get_track_addr(void* udata, msf_t* msf, int track) {
    psxd_cue_t* cue = udata;

    if (track > cue->num_tracks)
        return DISC_ERR_TRACK_OUT_OF_BOUNDS;
    
    msf->m = cue->track[track]->disc_offset.m;
    msf->s = cue->track[track]->disc_offset.s;

    return 0;
}

int psxd_cue_get_track_count(void* udata, int* count) {
    psxd_cue_t* cue = udata;

    *count = cue->num_tracks;

    return 0;
}

void psxd_cue_init_disc(psxd_cue_t* cue, psx_disc_t* disc) {
    disc->udata = cue;
    disc->seek_func = psxd_cue_seek;
    disc->read_sector_func = psxd_cue_read_sector;
    disc->get_track_addr_func = psxd_cue_get_track_addr;
    disc->get_track_count_func = psxd_cue_get_track_count;
}

void psxd_cue_destroy(psxd_cue_t* cue) {
    for (int i = 0; i < cue->num_tracks; i++) {
        free(cue->track[i]->filename);
        free(cue->track[i]);
    }

    free(cue->track);
    free(cue->current_file);
    free(cue->buf);
    free(cue);
}