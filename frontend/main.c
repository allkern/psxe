#include "psx/psx.h"

#include "screen.h"
#include "argparse.h"

#undef main

int main(int argc, const char* argv[]) {
    log_set_level(LOG_FATAL);

    psx_t* psx = psx_create();
    psx_init(psx);

    psx_load_bios(psx, "SCPH1001.bin");

    psx_gpu_t* gpu = psx_get_gpu(psx);

    psxe_screen_t* screen = psxe_screen_create();
    psxe_screen_init(screen, gpu);

    psx_gpu_set_dmode_event_callback(gpu, psxe_gpu_dmode_event_cb);
    psx_gpu_set_udata(gpu, 0, screen);

    if (argv[1]) {
        psx_load_exe(psx, argv[1]);
    }

    while (psxe_screen_is_open(screen)) {
        psx_run_frame(psx);

        psxe_screen_update(screen);
    }

    psx_destroy(psx);
    psxe_screen_destroy(screen);

    return 0;
}