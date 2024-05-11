#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>

#include "argparse.h"
#include "toml.h"

typedef struct {
    int use_args;
    int version;
    int help_model;
    int help_region;
    int log_level;
    int quiet;
    int console_source;
    int scale;
    const char* snap_path;
    const char* settings_path;
    const char* bios;
    const char* bios_search;
    const char* model;
    const char* exe;
    const char* region;
    const char* psxe_version;
    const char* cd_path;
    const char* exp_path;
} psxe_config_t;

psxe_config_t* psxe_cfg_create(void);
void psxe_cfg_init(psxe_config_t*);
void psxe_cfg_load_defaults(psxe_config_t*);
void psxe_cfg_load(psxe_config_t*, int, const char**);
char* psxe_cfg_get_bios_path(psxe_config_t*);
void psxe_cfg_destroy(psxe_config_t*);

#endif