#include "common.h"

#include "../psx/psx.h"
#include "../psx/log.h"
#include "../psx/dev/cdrom/cdrom.h"
#include "../psx/dev/gpu.h"
#include "../psx/dev/input.h"
#include "../psx/dev/pad.h"
#include "../psx/dev/spu.h"
#include "../psx/dev/timer.h"
#include "../psx/input/sda.h"

#include "libretro.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define AUDIO_SAMPLE_RATE 44100.0
#define FALLBACK_WIDTH 320
#define FALLBACK_HEIGHT 240

typedef struct {
    psx_t* psx;
    psx_input_t* input;
    char system_dir[PATH_MAX];
    char save_dir[PATH_MAX];
    char game_path[PATH_MAX];
    uint16_t* video_buf;
    size_t video_buf_pixels;
    int16_t* audio_buf;
    size_t audio_buf_frames;
    double audio_remainder;
    unsigned width;
    unsigned height;
    float aspect;
    unsigned port_device[2];
    bool frame_complete;
    bool game_loaded;
} psxe_libretro_t;

static psxe_libretro_t g_core;

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_log_printf_t log_cb;

static const struct retro_input_descriptor g_joypad_desc[] = {
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Square" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Cross" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Circle" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Triangle" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L1" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R1" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3" },
    { 0, 0, 0, 0, NULL }
};

static const struct retro_input_descriptor g_no_input_desc[] = {
    { 0, 0, 0, 0, NULL }
};

static void libretro_gpu_vblank_event_cb(psx_gpu_t* gpu) {
    g_core.frame_complete = true;
    psxe_gpu_vblank_timer_event_cb(gpu);
}

static void libretro_gpu_hblank_event_cb(psx_gpu_t* gpu) {
    psxe_gpu_hblank_event_cb(gpu);
}

static void libretro_gpu_hblank_end_event_cb(psx_gpu_t* gpu) {
    psxe_gpu_hblank_end_event_cb(gpu);
}

static void libretro_gpu_vblank_end_event_cb(psx_gpu_t* gpu) {
    psxe_gpu_vblank_end_event_cb(gpu);
}

static void fallback_log(enum retro_log_level level, const char* fmt, ...) {
    (void)level;

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static void core_log(enum retro_log_level level, const char* fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    if (log_cb) {
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, ap);
        log_cb(level, "%s", buffer);
    } else {
        vfprintf(stderr, fmt, ap);
    }

    va_end(ap);
}

static void path_join(char* out, size_t size, const char* dir, const char* file) {
    size_t len;

    if (!dir || !dir[0]) {
        snprintf(out, size, "%s", file ? file : "");
        return;
    }

    len = strlen(dir);

    if (dir[len - 1] == '/' || dir[len - 1] == '\\')
        snprintf(out, size, "%s%s", dir, file);
    else
        snprintf(out, size, "%s/%s", dir, file);
}

static const char* path_basename(const char* path) {
    const char* slash = strrchr(path, '/');
    const char* backslash = strrchr(path, '\\');
    const char* base = path;

    if (slash && slash > base)
        base = slash + 1;

    if (backslash && backslash > base)
        base = backslash + 1;

    return base;
}

static void path_stem(char* out, size_t size, const char* path) {
    const char* base = path_basename(path);
    const char* dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);

    if (len >= size)
        len = size - 1;

    memcpy(out, base, len);
    out[len] = '\0';
}

static bool file_exists(const char* path) {
    FILE* file;

    if (!path || !path[0])
        return false;

    file = fopen(path, "rb");

    if (!file)
        return false;

    fclose(file);

    return true;
}

static bool has_suffix_ci(const char* str, const char* suffix) {
    size_t str_len;
    size_t suffix_len;

    if (!str || !suffix)
        return false;

    str_len = strlen(str);
    suffix_len = strlen(suffix);

    if (suffix_len > str_len)
        return false;

    return strcasecmp(str + str_len - suffix_len, suffix) == 0;
}

static bool find_bios_path(char* out, size_t size) {
    static const char* bios_candidates[] = {
        "bios.bin",
        "scph1001.bin",
        "SCPH1001.BIN",
        "scph5501.bin",
        "scph5500.bin",
        "scph5502.bin",
        NULL
    };
    size_t i;
    DIR* dir;
    struct dirent* entry;

    if (!g_core.system_dir[0])
        return false;

    for (i = 0; bios_candidates[i]; ++i) {
        path_join(out, size, g_core.system_dir, bios_candidates[i]);

        if (file_exists(out))
            return true;
    }

    dir = opendir(g_core.system_dir);

    if (!dir)
        return false;

    while ((entry = readdir(dir))) {
        if ((entry->d_name[0] == '.') || !has_suffix_ci(entry->d_name, ".bin"))
            continue;

        if (!strncasecmp(entry->d_name, "scph", 4)) {
            path_join(out, size, g_core.system_dir, entry->d_name);
            closedir(dir);
            return true;
        }
    }

    closedir(dir);
    return false;
}

static const char* get_storage_dir(void) {
    if (g_core.save_dir[0])
        return g_core.save_dir;

    if (g_core.system_dir[0])
        return g_core.system_dir;

    return ".";
}

static void ensure_video_buffer(size_t pixels) {
    if (pixels <= g_core.video_buf_pixels)
        return;

    free(g_core.video_buf);
    g_core.video_buf = (uint16_t*)calloc(pixels, sizeof(uint16_t));
    g_core.video_buf_pixels = g_core.video_buf ? pixels : 0;
}

static void ensure_audio_buffer(size_t frames) {
    size_t samples = frames * 2;

    if (frames <= g_core.audio_buf_frames)
        return;

    free(g_core.audio_buf);
    g_core.audio_buf = (int16_t*)calloc(samples, sizeof(int16_t));
    g_core.audio_buf_frames = g_core.audio_buf ? frames : 0;
}

static unsigned get_display_width_or_default(void) {
    unsigned width = g_core.psx ? psx_get_display_width(g_core.psx) : 0;

    return width ? width : FALLBACK_WIDTH;
}

static unsigned get_display_height_or_default(void) {
    unsigned height = g_core.psx ? psx_get_display_height(g_core.psx) : 0;

    return height ? height : FALLBACK_HEIGHT;
}

static float get_display_aspect_or_default(void) {
    return 4.0f / 3.0f;
}

static void update_geometry(bool force) {
    struct retro_system_av_info av_info;
    bool changed = false;

    memset(&av_info, 0, sizeof(av_info));

    av_info.geometry.base_width = get_display_width_or_default();
    av_info.geometry.base_height = get_display_height_or_default();
    av_info.geometry.max_width = PSX_GPU_FB_WIDTH;
    av_info.geometry.max_height = PSX_GPU_FB_HEIGHT;
    av_info.geometry.aspect_ratio = get_display_aspect_or_default();
    av_info.timing.sample_rate = AUDIO_SAMPLE_RATE;
    av_info.timing.fps = retro_get_region() == RETRO_REGION_PAL ? 49.76 : 59.29;

    changed = force
        || av_info.geometry.base_width != g_core.width
        || av_info.geometry.base_height != g_core.height
        || av_info.geometry.aspect_ratio != g_core.aspect;

    if (!changed)
        return;

    g_core.width = av_info.geometry.base_width;
    g_core.height = av_info.geometry.base_height;
    g_core.aspect = av_info.geometry.aspect_ratio;

    environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info.geometry);
}

static void mix_audio(int16_t* buf, size_t frames) {
    size_t i;
    psx_cdrom_t* cdrom = g_core.psx->cdrom;
    psx_spu_t* spu = g_core.psx->spu;

    memset(buf, 0, frames * 2 * sizeof(*buf));

    psx_cdrom_get_audio_samples(cdrom, buf, frames * 2 * sizeof(*buf));
    psx_spu_update_cdda_buffer(spu, cdrom->cdda_buf);

    for (i = 0; i < frames; ++i) {
        uint32_t sample = psx_spu_get_sample(spu);
        int32_t left = buf[(i * 2) + 0] + (int16_t)(sample & 0xffff);
        int32_t right = buf[(i * 2) + 1] + (int16_t)(sample >> 16);

        if (left < -32768)
            left = -32768;
        else if (left > 32767)
            left = 32767;

        if (right < -32768)
            right = -32768;
        else if (right > 32767)
            right = 32767;

        buf[(i * 2) + 0] = (int16_t)left;
        buf[(i * 2) + 1] = (int16_t)right;
    }
}

static size_t get_audio_frames_for_run(void) {
    double fps = retro_get_region() == RETRO_REGION_PAL ? 49.76 : 59.29;
    double exact = (AUDIO_SAMPLE_RATE / fps) + g_core.audio_remainder;
    size_t frames = (size_t)exact;

    g_core.audio_remainder = exact - (double)frames;

    return frames ? frames : 1;
}

static uint16_t bgr555_to_rgb565(uint16_t pixel) {
    uint16_t red = pixel & 0x1f;
    uint16_t green = (pixel >> 5) & 0x1f;
    uint16_t blue = (pixel >> 10) & 0x1f;
    uint16_t green6 = (green << 1) | (green >> 4);

    return (uint16_t)((red << 11) | (green6 << 5) | blue);
}

static void convert_video_frame(void) {
    unsigned x, y;
    unsigned width = get_display_width_or_default();
    unsigned height = get_display_height_or_default();
    uint8_t is_rgb24 = (uint8_t)psx_get_display_format(g_core.psx);
    const uint8_t* src8;
    const uint16_t* src16;
    void* display_buf = psx_get_display_buffer(g_core.psx);

    if ((g_core.psx->gpu->disp_y + height) > PSX_GPU_FB_HEIGHT)
        display_buf = psx_get_vram(g_core.psx);

    ensure_video_buffer((size_t)width * height);

    if (!g_core.video_buf)
        return;

    if (is_rgb24) {
        src8 = (const uint8_t*)display_buf;

        for (y = 0; y < height; ++y) {
            const uint8_t* row = src8 + (y * PSX_GPU_FB_STRIDE);

            for (x = 0; x < width; ++x) {
                uint8_t red = row[(x * 3) + 0];
                uint8_t green = row[(x * 3) + 1];
                uint8_t blue = row[(x * 3) + 2];

                g_core.video_buf[(y * width) + x] = (uint16_t)(
                    ((red >> 3) << 11) |
                    ((green >> 2) << 5) |
                    (blue >> 3)
                );
            }
        }
    } else {
        src16 = (const uint16_t*)display_buf;

        for (y = 0; y < height; ++y) {
            const uint16_t* row = src16 + (y * (PSX_GPU_FB_STRIDE / sizeof(*row)));

            for (x = 0; x < width; ++x)
                g_core.video_buf[(y * width) + x] = bgr555_to_rgb565(row[x]);
        }
    }
}

static void update_input(void) {
    static const struct {
        unsigned id;
        uint32_t mask;
    } button_map[] = {
        { RETRO_DEVICE_ID_JOYPAD_B, PSXI_SW_SDA_CROSS },
        { RETRO_DEVICE_ID_JOYPAD_Y, PSXI_SW_SDA_SQUARE },
        { RETRO_DEVICE_ID_JOYPAD_X, PSXI_SW_SDA_TRIANGLE },
        { RETRO_DEVICE_ID_JOYPAD_A, PSXI_SW_SDA_CIRCLE },
        { RETRO_DEVICE_ID_JOYPAD_START, PSXI_SW_SDA_START },
        { RETRO_DEVICE_ID_JOYPAD_SELECT, PSXI_SW_SDA_SELECT },
        { RETRO_DEVICE_ID_JOYPAD_UP, PSXI_SW_SDA_PAD_UP },
        { RETRO_DEVICE_ID_JOYPAD_DOWN, PSXI_SW_SDA_PAD_DOWN },
        { RETRO_DEVICE_ID_JOYPAD_LEFT, PSXI_SW_SDA_PAD_LEFT },
        { RETRO_DEVICE_ID_JOYPAD_RIGHT, PSXI_SW_SDA_PAD_RIGHT },
        { RETRO_DEVICE_ID_JOYPAD_L, PSXI_SW_SDA_L1 },
        { RETRO_DEVICE_ID_JOYPAD_R, PSXI_SW_SDA_R1 },
        { RETRO_DEVICE_ID_JOYPAD_L2, PSXI_SW_SDA_L2 },
        { RETRO_DEVICE_ID_JOYPAD_R2, PSXI_SW_SDA_R2 },
        { RETRO_DEVICE_ID_JOYPAD_L3, PSXI_SW_SDA_L3 },
        { RETRO_DEVICE_ID_JOYPAD_R3, PSXI_SW_SDA_R3 }
    };
    size_t i;
    int16_t left_x;
    int16_t left_y;
    int16_t right_x;
    int16_t right_y;

    input_poll_cb();

    if (g_core.port_device[0] != RETRO_DEVICE_JOYPAD)
        return;

    for (i = 0; i < (sizeof(button_map) / sizeof(button_map[0])); ++i) {
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, button_map[i].id))
            psx_pad_button_press(g_core.psx->pad, 0, button_map[i].mask);
        else
            psx_pad_button_release(g_core.psx->pad, 0, button_map[i].mask);
    }

    left_x = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
    left_y = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
    right_x = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
    right_y = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

    psx_pad_analog_change(g_core.psx->pad, 0, PSXI_AX_SDA_LEFT_HORZ, (uint16_t)(((int32_t)left_x + 32768) >> 8));
    psx_pad_analog_change(g_core.psx->pad, 0, PSXI_AX_SDA_LEFT_VERT, (uint16_t)(((int32_t)left_y + 32768) >> 8));
    psx_pad_analog_change(g_core.psx->pad, 0, PSXI_AX_SDA_RIGHT_HORZ, (uint16_t)(((int32_t)right_x + 32768) >> 8));
    psx_pad_analog_change(g_core.psx->pad, 0, PSXI_AX_SDA_RIGHT_VERT, (uint16_t)(((int32_t)right_y + 32768) >> 8));
}

static void reset_core_state(void) {
    if (g_core.input) {
        psx_input_destroy(g_core.input);
    }

    if (g_core.psx) {
        psx_destroy(g_core.psx);
        g_core.psx = NULL;
    }

    g_core.input = NULL;

    free(g_core.video_buf);
    g_core.video_buf = NULL;
    g_core.video_buf_pixels = 0;

    free(g_core.audio_buf);
    g_core.audio_buf = NULL;
    g_core.audio_buf_frames = 0;

    g_core.audio_remainder = 0.0;
    g_core.width = FALLBACK_WIDTH;
    g_core.height = FALLBACK_HEIGHT;
    g_core.aspect = 4.0f / 3.0f;
    g_core.frame_complete = false;
    g_core.game_loaded = false;
    g_core.game_path[0] = '\0';
}

static bool init_system_paths(void) {
    const char* path = NULL;

    g_core.system_dir[0] = '\0';
    g_core.save_dir[0] = '\0';

    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &path) && path)
        snprintf(g_core.system_dir, sizeof(g_core.system_dir), "%s", path);

    path = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &path) && path)
        snprintf(g_core.save_dir, sizeof(g_core.save_dir), "%s", path);

    return true;
}

static bool attach_input_devices(void) {
    psxi_sda_t* controller;

    g_core.input = psx_input_create();

    if (!g_core.input)
        return false;

    psx_input_init(g_core.input);

    controller = psxi_sda_create();

    if (!controller)
        return false;

    psxi_sda_init(controller, SDA_MODEL_DIGITAL);
    psxi_sda_init_input(controller, g_core.input);

    psx_pad_attach_joy(g_core.psx->pad, 0, g_core.input);
    g_core.input = NULL;

    return true;
}

static void attach_memory_cards(void) {
    char stem[PATH_MAX];
    char slot1[PATH_MAX];
    char slot2[PATH_MAX];
    const char* storage_dir = get_storage_dir();

    path_stem(stem, sizeof(stem), g_core.game_path);
    path_join(slot1, sizeof(slot1), storage_dir, stem[0] ? stem : "psxe");
    path_join(slot2, sizeof(slot2), storage_dir, stem[0] ? stem : "psxe");

    strncat(slot1, ".slot1.mcd", sizeof(slot1) - strlen(slot1) - 1);
    strncat(slot2, ".slot2.mcd", sizeof(slot2) - strlen(slot2) - 1);

    psx_pad_attach_mcd(g_core.psx->pad, 0, slot1);
    psx_pad_attach_mcd(g_core.psx->pad, 1, slot2);
}

unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

void retro_set_environment(retro_environment_t cb) {
    static const struct retro_controller_description controller_types[] = {
        { "PlayStation Pad", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
        { NULL, 0 }
    };
    static const struct retro_controller_info ports[] = {
        { controller_types, 1 },
        { NULL, 0 }
    };
    bool no_game = false;

    environ_cb = cb;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
    cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
    video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

void retro_init(void) {
    struct retro_log_callback logging;
    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;

    memset(&g_core, 0, sizeof(g_core));

    log_cb = fallback_log;

    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;

    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
        core_log(RETRO_LOG_WARN, "libretro frontend did not accept RGB565 pixel format\n");

    g_core.width = FALLBACK_WIDTH;
    g_core.height = FALLBACK_HEIGHT;
    g_core.aspect = 4.0f / 3.0f;
    g_core.port_device[0] = RETRO_DEVICE_JOYPAD;
    g_core.port_device[1] = RETRO_DEVICE_NONE;

    log_set_quiet(true);
    init_system_paths();
    retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
}

void retro_deinit(void) {
    reset_core_state();
}

void retro_get_system_info(struct retro_system_info* info) {
    memset(info, 0, sizeof(*info));
    info->library_name = "psxe";
    info->library_version = PSXE_VERSION;
    info->need_fullpath = true;
    info->block_extract = true;
    info->valid_extensions = "cue";
}

void retro_get_system_av_info(struct retro_system_av_info* info) {
    memset(info, 0, sizeof(*info));

    info->geometry.base_width = g_core.width ? g_core.width : FALLBACK_WIDTH;
    info->geometry.base_height = g_core.height ? g_core.height : FALLBACK_HEIGHT;
    info->geometry.max_width = PSX_GPU_FB_WIDTH;
    info->geometry.max_height = PSX_GPU_FB_HEIGHT;
    info->geometry.aspect_ratio = g_core.aspect > 0.0f ? g_core.aspect : (4.0f / 3.0f);
    info->timing.fps = retro_get_region() == RETRO_REGION_PAL ? 49.76 : 59.29;
    info->timing.sample_rate = AUDIO_SAMPLE_RATE;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
    if (port >= 2)
        return;

    g_core.port_device[port] = device & RETRO_DEVICE_MASK;

    if (port == 0) {
        if (g_core.port_device[port] == RETRO_DEVICE_JOYPAD)
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)g_joypad_desc);
        else
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)g_no_input_desc);
    }
}

void retro_reset(void) {
    if (g_core.psx)
        psx_soft_reset(g_core.psx);
}

bool retro_load_game(const struct retro_game_info* game) {
    char bios_path[PATH_MAX];

    if (!game || !game->path)
        return false;

    reset_core_state();
    init_system_paths();

    snprintf(g_core.game_path, sizeof(g_core.game_path), "%s", game->path);

    if (!find_bios_path(bios_path, sizeof(bios_path))) {
        core_log(RETRO_LOG_ERROR, "no PlayStation BIOS found in %s\n", g_core.system_dir[0] ? g_core.system_dir : "(no system dir)");
        return false;
    }

    g_core.psx = psx_create();

    if (!g_core.psx)
        return false;

    if (psx_init(g_core.psx, bios_path, NULL)) {
        core_log(RETRO_LOG_ERROR, "failed to initialize BIOS from %s\n", bios_path);
        free(g_core.psx);
        g_core.psx = NULL;
        return false;
    }

    core_log(RETRO_LOG_INFO, "using BIOS %s\n", bios_path);

    psx_gpu_set_event_callback(g_core.psx->gpu, GPU_EVENT_VBLANK, libretro_gpu_vblank_event_cb);
    psx_gpu_set_event_callback(g_core.psx->gpu, GPU_EVENT_HBLANK, libretro_gpu_hblank_event_cb);
    psx_gpu_set_event_callback(g_core.psx->gpu, GPU_EVENT_HBLANK_END, libretro_gpu_hblank_end_event_cb);
    psx_gpu_set_event_callback(g_core.psx->gpu, GPU_EVENT_VBLANK_END, libretro_gpu_vblank_end_event_cb);
    psx_gpu_set_udata(g_core.psx->gpu, 1, g_core.psx->timer);

    if (!attach_input_devices()) {
        core_log(RETRO_LOG_ERROR, "failed to create controller input\n");
        reset_core_state();
        return false;
    }

    attach_memory_cards();

    if (psx_cdrom_open(g_core.psx->cdrom, game->path) != CDT_LICENSED) {
        core_log(RETRO_LOG_ERROR, "failed to open disc image %s\n", game->path);
        reset_core_state();
        return false;
    }

    g_core.game_loaded = true;
    update_geometry(true);

    return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info) {
    (void)game_type;
    (void)info;
    (void)num_info;
    return false;
}

void retro_unload_game(void) {
    reset_core_state();
}

unsigned retro_get_region(void) {
    if (g_core.psx && (g_core.psx->gpu->display_mode & 0x8))
        return RETRO_REGION_PAL;

    return RETRO_REGION_NTSC;
}

void* retro_get_memory_data(unsigned id) {
    (void)id;
    return NULL;
}

size_t retro_get_memory_size(unsigned id) {
    (void)id;
    return 0;
}

size_t retro_serialize_size(void) {
    return 0;
}

bool retro_serialize(void* data, size_t size) {
    (void)data;
    (void)size;
    return false;
}

bool retro_unserialize(const void* data, size_t size) {
    (void)data;
    (void)size;
    return false;
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned index, bool enabled, const char* code) {
    (void)index;
    (void)enabled;
    (void)code;
}

void retro_run(void) {
    size_t frames;

    if (!g_core.game_loaded || !g_core.psx) {
        video_cb(NULL, FALLBACK_WIDTH, FALLBACK_HEIGHT, 0);
        return;
    }

    update_input();
    g_core.frame_complete = false;

    while (!g_core.frame_complete)
        psx_update(g_core.psx);

    update_geometry(false);
    convert_video_frame();

    if (g_core.video_buf)
        video_cb(g_core.video_buf, g_core.width, g_core.height, g_core.width * sizeof(*g_core.video_buf));
    else
        video_cb(NULL, g_core.width, g_core.height, 0);

    frames = get_audio_frames_for_run();
    ensure_audio_buffer(frames);

    if (g_core.audio_buf) {
        mix_audio(g_core.audio_buf, frames);
        audio_batch_cb(g_core.audio_buf, frames);
    } else {
        size_t i;

        for (i = 0; i < frames; ++i)
            audio_cb(0, 0);
    }
}
