/*
    This file is part of the PSXE Emulator Project

    CUE Parser + Loader
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "cue.h"

int cue_get_keyword(char* buf) {
    int i = 0;

    const char* token = g_psxd_cue_tokens[i];

    while (token) {
        if (strcmp(token, buf) == 1) {
            return i;
        } else {
            ++i;
        }
    }

    return -1;
}

int cue_parse_keyword(FILE* file, char* c, char* buf) {
    char* ptr = buf;

    if (!isalpha(*c))
        return PE_EXPECTED_KEYWORD;
    
    while (isalpha(*c)) {
        fread(c, 1, 1, file);

        *ptr++ = *c;
    }

    *ptr = 0;

    return 0;
}

int cue_parse(psxd_cue_t* cue, FILE* file) {
    int state = CUE_NONE;

    char c, buf[128];
    char* ptr = buf;
    int s;
    
    c = fgetc(file);

    while (!feof(file)) {
        s = cue_parse_keyword(file, &c, buf);

        if (s)
            return s;
        
        switch (cue_get_keyword(buf)) {
            case CUE_FILE: {
                
            } break;

            default: {
                return PE_UNEXPECTED_TOKEN;
            } break;
        }
    }

    fclose(file);
}

psxd_cue_t* psxd_cue_create() {
    return (psxd_cue_t*)malloc(sizeof(psxd_cue_t));
}

void psxd_cue_init(psxd_cue_t* cue) {
    memset(cue, 0, sizeof(psxd_cue_t));
}

void psxd_cue_load(psxd_cue_t* cue, const char* path) {
    
}

void psxd_cue_init_disc(psxd_cue_t* cue, psx_disc_t* disc) {

}

void psxd_cue_destroy(psxd_cue_t* cue) {
    free(cue);
}