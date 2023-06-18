#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdlib.h>

typedef struct {
    int         use_args;
    int         version;
    int         help_model;
    int         help_region;
    const char* settings_path;
    const char* bios;
    const char* bios_search;
    const char* model;
    const char* exe;
    const char* region;
    const char* psxe_version;
} psxe_settings_t;

psxe_settings_t* psxe_settings_create();
void psxe_settings_init(psxe_settings_t*);
void psxe_settings_load_defaults(psxe_settings_t*);
char* psxe_settings_get_bios_path(psxe_settings_t*);

#endif