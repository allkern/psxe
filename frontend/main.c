#include "psx/psx.h"

#include "screen.h"
#include "argparse.h"

#undef main

int main(int argc, const char* argv[]) {
    log_set_level(LOG_FATAL);

    psx_t* psx = psx_create();
    psx_init(psx, "SCPH1001.bin");

    psx_gpu_t* gpu = psx_get_gpu(psx);

    psxe_screen_t* screen = psxe_screen_create();
    psxe_screen_init(screen, gpu);
    psxe_screen_set_scale(screen, 2);
    psxe_screen_reload(screen);

    psx_gpu_set_event_callback(gpu, GPU_EVENT_DMODE, psxe_gpu_dmode_event_cb);
    psx_gpu_set_udata(gpu, 0, screen);

    if (argv[1]) {
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