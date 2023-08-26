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
    ++cue->numtracks;

    cue->track = realloc(cue->track, cue->numtracks * sizeof(cue_track_t*));
    cue->track[cue->numtracks - 1] = malloc(sizeof(cue_track_t));

    memset(cue->track[cue->numtracks - 1], 0, sizeof(cue_track_t));
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

    // Open file, get size and seek to 0
    FILE* trackfile = fopen(cue->buf, "rb");

    fseek(trackfile, 0, SEEK_END);

    filesz = ftell(trackfile);

    fseek(trackfile, 0, SEEK_SET);

    // If we have to preload the disc image
    // then copy data to our filebuf and close
    // the file. Otherwise, our filebuf contains
    // a pointer to the open FILE.
    if (cue->preload) {
        filebuf = malloc(filesz);

        fread(filebuf, 1, filesz, trackfile);

        fclose(trackfile);
    } else {
        filebuf = trackfile;
    }

    strcpy(cue->current_file, cue->buf);

    EXPECT_KEYWORD(CUE_BINARY);
    EXPECT_KEYWORD(CUE_TRACK);

    if (cue_parse_number(cue))
        return cue->error;
    
    int track = atoi(cue->buf) - 1;
    
    if (track != cue->numtracks)
        ERROR_OUT(PE_NON_SEQUENTIAL_TRACKS);
    
    cue_add_track(cue);

    cue->track[track]->buf = filebuf;
    cue->track[track]->size = filesz;
    cue->track[track]->filename = malloc(strlen(cue->current_file));

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

void psxd_cue_load(psxd_cue_t* cue, const char* path) {
    FILE* file = fopen(path, "rb");

    cue_parse(cue, file);

    if (cue->error) {
        log_fatal("CUE error %u", cue->error);
    }
}

void psxd_cue_init_disc(psxd_cue_t* cue, psx_disc_t* disc) {

}

void psxd_cue_destroy(psxd_cue_t* cue) {
    free(cue->current_file);
    free(cue->buf);
    free(cue);
}