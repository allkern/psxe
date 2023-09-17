/*
    This file is part of the PSXE Emulator Project

    CUE Parser + Loader
*/

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "cue.h"
#include "../log.h"

#define CUE_BUF_SIZE 256

static const char* g_psxd_cue_errors[] = {
    "PE_NO_ERROR",
    "PE_EXPECTED_KEYWORD",
    "PE_EXPECTED_STRING",
    "PE_EXPECTED_NUMBER",
    "PE_EXPECTED_COLON",
    "PE_NON_SEQUENTIAL_TRACKS",
    "PE_UNEXPECTED_TOKEN"
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

void psxd_cue_init(psxd_cue_t* cue) {
    memset(cue, 0, sizeof(psxd_cue_t));

    cue->buf = malloc(CUE_BUF_SIZE);
    cue->ptr = cue->buf;
    cue->current_file = malloc(CUE_BUF_SIZE);
}

char* cue_get_directory(const char* path) {
    const char* ptr = &path[strlen(path) - 1];
    char* dir = NULL;

    while ((*ptr != '/') && (*ptr != '\\') && (ptr != path))
        ptr--;
    
    // If no directory specified, assume CWD
    if (ptr == path) {
        dir = malloc(3);

        strcpy(dir, ".\\");

        return dir;
    }
    
    ptrdiff_t len = (ptr - path) + 1;

    dir = malloc(len + 1);

    strncpy(dir, path, len);

    dir[len] = 0;

    return dir;
}

int psxd_cue_load(psxd_cue_t* cue, const char* path) {
    log_fatal("Parsing CUE...");

    FILE* file = fopen(path, "rb");

    if (ferror(file) || !file) {
        fclose(file);

        return 1;
    }

    if (cue_parse(cue, file)) {
        log_fatal("CUE error %s (%u)",
            g_psxd_cue_errors[cue->error],
            cue->error
        );

        exit(1);
    }

    log_fatal("Loading CD image...");

    size_t offset = 0;

    char* directory = cue_get_directory(path);
    size_t directory_len = strlen(directory);

    for (int i = 0; i < cue->num_tracks; i++) {
        cue_track_t* track = cue->track[i];

        int len = strlen(track->filename) + directory_len;

        char* full_path = malloc(len + 2);

        strcpy(full_path, directory);
        strcat(full_path, track->filename);

        log_fatal("Loading track %u at \'%s\'...", i + 1, full_path);

        FILE* track_file = fopen(full_path, "rb");

        if (ferror(track_file) || !track_file) {
            fclose(track_file);

            return 1;
        }

        uint32_t data_offset = msf_to_address(track->index[1]);

        // Get track size
        fseek(track_file, 0, SEEK_END);

        // Account for index 1 offset
        track->size = ftell(track_file);

        cue->buf_size += track->size;

        // Calculate track MS(F)
        msf_from_address(&track->disc_offset, offset + data_offset);
        msf_add_s(&track->disc_offset, 2);

        cue->buf = cue_alloc_block(cue->buf, &offset, track->size);

        fseek(track_file, 0, SEEK_SET);
        fread(cue->buf + (offset - track->size), 1, track->size, track_file);

        fclose(track_file);
        free(full_path);
    }

    // Calculate disc end MSF
    msf_from_address(&cue->end, offset);
    msf_add_s(&cue->end, 2);

    free(directory);

    log_fatal("Loaded CUE image, size=%08x, end=%02u:%02u:%02u",
        cue->buf_size,
        cue->end.m,
        cue->end.s,
        cue->end.f
    );

    return 0;
}

int psxd_cue_seek(void* udata, msf_t msf) {
    psxd_cue_t* cue = udata;

    // To-do: Check for OOB seeks

    uint32_t sectors = (((msf.m * 60) + msf.s - 2) * CD_SECTORS_PS) + msf.f;

    cue->seek_offset = sectors * CD_SECTOR_SIZE;

    log_fatal("CUE seek to %02u:%02u:%02u (%08x < %08x)", msf.m, msf.s, msf.f, cue->seek_offset, cue->buf_size);

    if (cue->seek_offset >= cue->buf_size)
        return DISC_ERR_ADDR_OUT_OF_BOUNDS;

    return 0;
}

int psxd_cue_read_sector(void* udata, void* buf) {
    psxd_cue_t* cue = udata;

    log_fatal("Reading sector at offset %08x", cue->seek_offset);

    memcpy(buf, cue->buf + cue->seek_offset, CD_SECTOR_SIZE);

    return 0;
}

int psxd_cue_get_track_addr(void* udata, msf_t* msf, int track) {
    psxd_cue_t* cue = udata;

    if (!track) {
        msf->m = cue->end.m;
        msf->s = cue->end.s;

        return 0;
    }

    if (track > cue->num_tracks)
        return DISC_ERR_TRACK_OUT_OF_BOUNDS;
    
    msf->m = cue->track[track - 1]->disc_offset.m;
    msf->s = cue->track[track - 1]->disc_offset.s;

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
    disc->destroy_func = (disc_destroy_t)psxd_cue_destroy;
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