#include "psx/psx.h"

#include "screen.h"
#include "config.h"

#include <signal.h>

#undef main

psx_cpu_t* g_cpu;

void sigint_handler(int sig) {
    log_fatal("r0=%08x at=%08x v0=%08x v1=%08x", g_cpu->r[0] , g_cpu->r[1] , g_cpu->r[2] , g_cpu->r[3] );
    log_fatal("a0=%08x a1=%08x a2=%08x a3=%08x", g_cpu->r[4] , g_cpu->r[5] , g_cpu->r[6] , g_cpu->r[7] );
    log_fatal("t0=%08x t1=%08x t2=%08x t3=%08x", g_cpu->r[8] , g_cpu->r[9] , g_cpu->r[10], g_cpu->r[11]);
    log_fatal("t4=%08x t5=%08x t6=%08x t7=%08x", g_cpu->r[12], g_cpu->r[13], g_cpu->r[14], g_cpu->r[15]);
    log_fatal("s0=%08x s1=%08x s2=%08x s3=%08x", g_cpu->r[16], g_cpu->r[17], g_cpu->r[18], g_cpu->r[19]);
    log_fatal("s4=%08x s5=%08x s6=%08x s7=%08x", g_cpu->r[20], g_cpu->r[21], g_cpu->r[22], g_cpu->r[23]);
    log_fatal("t8=%08x t9=%08x k0=%08x k1=%08x", g_cpu->r[24], g_cpu->r[25], g_cpu->r[26], g_cpu->r[27]);
    log_fatal("gp=%08x sp=%08x fp=%08x ra=%08x", g_cpu->r[28], g_cpu->r[29], g_cpu->r[30], g_cpu->r[31]);
    log_fatal("pc=%08x hi=%08x lo=%08x", g_cpu->pc, g_cpu->hi, g_cpu->lo);

    exit(1);
}

int main(int argc, const char* argv[]) {
    psxe_config_t* cfg = psxe_cfg_create();

    psxe_cfg_init(cfg);
    psxe_cfg_load_defaults(cfg);
    psxe_cfg_load(cfg, argc, argv);
    

    log_set_level(LOG_FATAL);

    signal(SIGINT, sigint_handler);

    psx_t* psx = psx_create();
    psx_init(psx, "SCPH1001.BIN");

    psx_gpu_t* gpu = psx_get_gpu(psx);

    g_cpu = psx_get_cpu(psx);

    psxe_screen_t* screen = psxe_screen_create();
    psxe_screen_init(screen, psx);
    psxe_screen_set_scale(screen, 2);
    psxe_screen_reload(screen);

    psx_gpu_set_event_callback(gpu, GPU_EVENT_DMODE, psxe_gpu_dmode_event_cb);
    psx_gpu_set_udata(gpu, 0, screen);

    if (argv[1]) {
        while (psx->cpu->pc != 0x80030008) {
            psx_update(psx);
        }

        psx_load_exe(psx, argv[1]);
    }

    while (psxe_screen_is_open(screen)) {
        psx_run_frame(psx);

        psxe_screen_update(screen);
    }

    psx_cpu_t* cpu = psx_get_cpu(psx);

    log_fatal("r0=%08x at=%08x v0=%08x v1=%08x", cpu->r[0] , cpu->r[1] , cpu->r[2] , cpu->r[3] );
    log_fatal("a0=%08x a1=%08x a2=%08x a3=%08x", cpu->r[4] , cpu->r[5] , cpu->r[6] , cpu->r[7] );
    log_fatal("t0=%08x t1=%08x t2=%08x t3=%08x", cpu->r[8] , cpu->r[9] , cpu->r[10], cpu->r[11]);
    log_fatal("t4=%08x t5=%08x t6=%08x t7=%08x", cpu->r[12], cpu->r[13], cpu->r[14], cpu->r[15]);
    log_fatal("s0=%08x s1=%08x s2=%08x s3=%08x", cpu->r[16], cpu->r[17], cpu->r[18], cpu->r[19]);
    log_fatal("s4=%08x s5=%08x s6=%08x s7=%08x", cpu->r[20], cpu->r[21], cpu->r[22], cpu->r[23]);
    log_fatal("t8=%08x t9=%08x k0=%08x k1=%08x", cpu->r[24], cpu->r[25], cpu->r[26], cpu->r[27]);
    log_fatal("gp=%08x sp=%08x fp=%08x ra=%08x", cpu->r[28], cpu->r[29], cpu->r[30], cpu->r[31]);
    log_fatal("pc=%08x hi=%08x lo=%08x", cpu->pc, cpu->hi, cpu->lo);

    psx_destroy(psx);
    psxe_screen_destroy(screen);

    return 0;
}