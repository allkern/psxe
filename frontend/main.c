#include "psx/psx.h"
#include "psx/input/sda.h"

#include "screen.h"
#include "config.h"

#undef main

int main(int argc, const char* argv[]) {
    psxe_config_t* cfg = psxe_cfg_create();

    psxe_cfg_init(cfg);
    psxe_cfg_load_defaults(cfg);
    psxe_cfg_load(cfg, argc, argv);

    log_set_level(cfg->log_level);

    psx_t* psx = psx_create();
    psx_init(psx, cfg->bios);

    psx_cdrom_t* cdrom = psx_get_cdrom(psx);

    if (cfg->cd_path)
        psx_cdrom_open(cdrom, cfg->cd_path);

    psxe_screen_t* screen = psxe_screen_create();
    psxe_screen_init(screen, psx);
    psxe_screen_set_scale(screen, 2);
    psxe_screen_reload(screen);

    psx_gpu_t* gpu = psx_get_gpu(psx);
    psx_gpu_set_event_callback(gpu, GPU_EVENT_DMODE, psxe_gpu_dmode_event_cb);
    psx_gpu_set_event_callback(gpu, GPU_EVENT_VBLANK, psxe_gpu_vblank_event_cb);
    psx_gpu_set_event_callback(gpu, GPU_EVENT_HBLANK, psxe_gpu_hblank_event_cb);
    psx_gpu_set_event_callback(gpu, GPU_EVENT_VBLANK_END, psxe_gpu_vblank_end_event_cb);
    psx_gpu_set_event_callback(gpu, GPU_EVENT_HBLANK_END, psxe_gpu_hblank_end_event_cb);
    psx_gpu_set_udata(gpu, 0, screen);
    psx_gpu_set_udata(gpu, 1, psx->timer);

    psx_input_t* input = psx_input_create();
    psx_input_init(input);

    psxi_sda_t* controller = psxi_sda_create();
    psxi_sda_init(controller, SDA_MODEL_DIGITAL);
    psxi_sda_init_input(controller, input);

    psx_pad_init_slot(psx->pad, 0, input);

    if (cfg->exe) {
        while (psx->cpu->pc != 0x80030000) {
            psx_update(psx);
        }

        psx_load_exe(psx, cfg->exe);
    }

    while (psxe_screen_is_open(screen)) {
        psx_update(psx);
    }

    psx_cpu_t* cpu = psx_get_cpu(psx);

    log_set_quiet(0);

    log_fatal("r0=%08x at=%08x v0=%08x v1=%08x", cpu->r[0] , cpu->r[1] , cpu->r[2] , cpu->r[3] );
    log_fatal("a0=%08x a1=%08x a2=%08x a3=%08x", cpu->r[4] , cpu->r[5] , cpu->r[6] , cpu->r[7] );
    log_fatal("t0=%08x t1=%08x t2=%08x t3=%08x", cpu->r[8] , cpu->r[9] , cpu->r[10], cpu->r[11]);
    log_fatal("t4=%08x t5=%08x t6=%08x t7=%08x", cpu->r[12], cpu->r[13], cpu->r[14], cpu->r[15]);
    log_fatal("s0=%08x s1=%08x s2=%08x s3=%08x", cpu->r[16], cpu->r[17], cpu->r[18], cpu->r[19]);
    log_fatal("s4=%08x s5=%08x s6=%08x s7=%08x", cpu->r[20], cpu->r[21], cpu->r[22], cpu->r[23]);
    log_fatal("t8=%08x t9=%08x k0=%08x k1=%08x", cpu->r[24], cpu->r[25], cpu->r[26], cpu->r[27]);
    log_fatal("gp=%08x sp=%08x fp=%08x ra=%08x", cpu->r[28], cpu->r[29], cpu->r[30], cpu->r[31]);
    log_fatal("pc=%08x hi=%08x lo=%08x ep=%08x", cpu->pc, cpu->hi, cpu->lo, cpu->cop0_r[COP0_EPC]);

    if (cfg->cd_path)
        psx_cdrom_close(cdrom);

    psx_input_destroy(input);
    psx_destroy(psx);
    psxi_sda_destroy(controller);
    psxe_screen_destroy(screen);

    return 0;
}