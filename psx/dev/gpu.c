#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "gpu.h"
#include "../log.h"

#define SE10(v) ((int16_t)((v) << 5) >> 5)

int g_psx_gpu_dither_kernel[] = {
    -4, +0, -3, +1,
    +2, -2, +3, -1,
    -3, +1, -4, +0,
    +3, -1, +2, -2,
};

uint16_t gpu_to_bgr555(uint32_t color) {
    return ((color & 0x0000f8) >> 3) |
           ((color & 0x00f800) >> 6) |
           ((color & 0xf80000) >> 9);
}

#define BGR555(c) \
    (((c & 0x0000f8) >> 3) | \
     ((c & 0x00f800) >> 6) | \
     ((c & 0xf80000) >> 9))

// #define BGR555(c) gpu_to_bgr555(c)

int min3(int a, int b, int c) {
    int m = (a <= b) ? a : b;

    return (m <= c) ? m : c;
}

int max3(int a, int b, int c) {
    int m = (a > b) ? a : b;

    return (m > c) ? m : c;
}

psx_gpu_t* psx_gpu_create(void) {
    return (psx_gpu_t*)malloc(sizeof(psx_gpu_t));
}

void psx_gpu_init(psx_gpu_t* gpu, psx_ic_t* ic) {
    memset(gpu, 0, sizeof(psx_gpu_t));

    gpu->io_base = PSX_GPU_BEGIN;
    gpu->io_size = PSX_GPU_SIZE;

    gpu->vram = (uint16_t*)malloc(PSX_GPU_VRAM_SIZE);
    gpu->empty = malloc(PSX_GPU_VRAM_SIZE);

    memset(gpu->empty, 0, PSX_GPU_VRAM_SIZE);

    gpu->state = GPU_STATE_RECV_CMD;
    gpu->gpustat |= 0x800000;

    // Default window size, this is not normally needed
    gpu->display_mode = 1;

    gpu->ic = ic;
}

uint32_t psx_gpu_read32(psx_gpu_t* gpu, uint32_t offset) {
    switch (offset) {
        case 0x00: {
            uint32_t data = 0x0;

            if (gpu->c0_tsiz) {
                data |= gpu->vram[gpu->c0_addr + (gpu->c0_xcnt + (gpu->c0_ycnt * 1024))];

                gpu->c0_xcnt += 1;

                if (gpu->c0_xcnt == gpu->c0_xsiz) {
                    gpu->c0_ycnt += 1;
                    gpu->c0_xcnt = 0;
                }

                data |= gpu->vram[gpu->c0_addr + (gpu->c0_xcnt + (gpu->c0_ycnt * 1024))] << 16;

                gpu->c0_xcnt += 1;

                if (gpu->c0_xcnt == gpu->c0_xsiz) {
                    gpu->c0_ycnt += 1;
                    gpu->c0_xcnt = 0;
                }

                gpu->c0_tsiz -= 2;
            }

            if (gpu->gp1_10h_req) {
                switch (gpu->gp1_10h_req & 7) {
                    case 2: {
                        data = ((gpu->texw_oy / 8) << 15) | ((gpu->texw_ox / 8) << 10) | ((gpu->texw_my / 8) << 5) | (gpu->texw_mx / 8);
                    } break;
                    case 3: {
                        data = (gpu->draw_y1 << 10) | gpu->draw_x1;
                    } break;
                    case 4: {
                        data = (gpu->draw_y2 << 10) | gpu->draw_x2;
                    } break;
                    case 5: {
                        data = (gpu->off_y << 10) | gpu->off_x;
                    } break;
                }

                gpu->gp1_10h_req = 0;
            }

            return data;
        } break;
        case 0x04: return gpu->gpustat | 0x1c000000;
    }

    log_warn("Unhandled 32-bit GPU read at offset %08x", offset);

    return 0x0;
}

uint16_t psx_gpu_read16(psx_gpu_t* gpu, uint32_t offset) {
    printf("Unhandled 16-bit GPU read at offset %08x\n", offset);

    return 0;

    // exit(1);
}

uint8_t psx_gpu_read8(psx_gpu_t* gpu, uint32_t offset) {
    printf("Unhandled 8-bit GPU read at offset %08x\n", offset);

    return 0;

    // exit(1);
}

int min(int x0, int x1) {
    return (x0 <= x1) ? x0 : x1;
}

int max(int x0, int x1) {
    return (x0 >= x1) ? x0 : x1;
}

#define EDGE(a, b, c) ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x))

uint16_t gpu_fetch_texel(psx_gpu_t* gpu, uint16_t tx, uint16_t ty, uint32_t tpx, uint32_t tpy, uint16_t clutx, uint16_t cluty, int depth) {
    tx = (tx & ~gpu->texw_mx) | (gpu->texw_ox & gpu->texw_mx);
    ty = (ty & ~gpu->texw_my) | (gpu->texw_oy & gpu->texw_my);
    tx &= 0xff;
    ty &= 0xff;

    switch (depth) {
        // 4-bit
        case 0: {
            uint16_t texel = gpu->vram[(tpx + (tx >> 2)) + ((tpy + ty) * 1024)];

            int index = (texel >> ((tx & 0x3) << 2)) & 0xf;

            return gpu->vram[(clutx + index) + (cluty * 1024)];
        } break;

        // 8-bit
        case 1: {
            uint16_t texel = gpu->vram[(tpx + (tx >> 1)) + ((tpy + ty) * 1024)];

            int index = (texel >> ((tx & 0x1) << 3)) & 0xff;

            return gpu->vram[(clutx + index) + (cluty * 1024)];
        } break;

        // 15-bit
        default: {
            return gpu->vram[(tpx + tx) + ((tpy + ty) * 1024)];
        } break;
    }
}

uint16_t gpu_fetch_texel_bilinear(psx_gpu_t* gpu, float tx, float ty, uint32_t tpx, uint32_t tpy, uint16_t clutx, uint16_t cluty, int depth) {
    float txf = floorf(tx);
    float tyf = floorf(ty);
    float txc = txf + 1.0f;
    float tyc = tyf + 1.0f;

    int s0 = gpu_fetch_texel(gpu, (int)txf, (int)tyf, tpx, tpy, clutx, cluty, depth);

    if (!s0)
        return 0;

    int s1 = gpu_fetch_texel(gpu, (int)txc, (int)tyf, tpx, tpy, clutx, cluty, depth);
    int s2 = gpu_fetch_texel(gpu, (int)txf, (int)tyc, tpx, tpy, clutx, cluty, depth);
    int s3 = gpu_fetch_texel(gpu, (int)txc, (int)tyc, tpx, tpy, clutx, cluty, depth);

    float s0r = (s0 >> 0) & 0x1f;
    float s0g = (s0 >> 5) & 0x1f;
    float s0b = (s0 >> 10) & 0x1f;
    float s1r = (s1 >> 0) & 0x1f;
    float s1g = (s1 >> 5) & 0x1f;
    float s1b = (s1 >> 10) & 0x1f;
    float s2r = (s2 >> 0) & 0x1f;
    float s2g = (s2 >> 5) & 0x1f;
    float s2b = (s2 >> 10) & 0x1f;
    float s3r = (s3 >> 0) & 0x1f;
    float s3g = (s3 >> 5) & 0x1f;
    float s3b = (s3 >> 10) & 0x1f;

    float q1r = s0r * (txc - tx) + s1r * (tx - txf);
    float q1g = s0g * (txc - tx) + s1g * (tx - txf);
    float q1b = s0b * (txc - tx) + s1b * (tx - txf);
    float q2r = s2r * (txc - tx) + s3r * (tx - txf);
    float q2g = s2g * (txc - tx) + s3g * (tx - txf);
    float q2b = s2b * (txc - tx) + s3b * (tx - txf);
    int qr = q1r * (tyc - ty) + q2r * (ty - tyf);
    int qg = q1g * (tyc - ty) + q2g * (ty - tyf);
    int qb = q1b * (tyc - ty) + q2b * (ty - tyf);

    return qr | (qg << 5) | (qb << 10) | (s0 & 0x8000) | (s1 & 0x8000) | (s2 & 0x8000) | (s3 & 0x8000);
}

#define TL(z, a, b) \
    ((z < 0) || ((z == 0) && ((b.y > a.y) || ((b.y == a.y) && (b.x < a.x)))))

void gpu_render_triangle(psx_gpu_t* gpu, vertex_t v0, vertex_t v1, vertex_t v2, poly_data_t data, int edge) {
    vertex_t a, b, c, p;

    int tpx = (data.texp & 0xf) << 6;
    int tpy = (data.texp & 0x10) << 4;
    int clutx = (data.clut & 0x3f) << 4;
    int cluty = (data.clut >> 6) & 0x1ff;
    int depth = (data.texp >> 7) & 3;
    int transp = (data.attrib & PA_TRANSP) != 0;
    int transp_mode;

    if (data.attrib & PA_TEXTURED) {
        transp_mode = (data.texp >> 5) & 3;
    } else {
        transp_mode = (gpu->gpustat >> 5) & 3;
    }

    a = v0;

    /* Ensure the winding order is correct */
    if (EDGE(v0, v1, v2) < 0) {
        b = v2;
        c = v1;
    } else {
        b = v1;
        c = v2;
    }

    a.x += gpu->off_x;
    b.x += gpu->off_x;
    c.x += gpu->off_x;
    a.y += gpu->off_y;
    b.y += gpu->off_y;
    c.y += gpu->off_y;

    int xmin = min3(a.x, b.x, c.x);
    int ymin = min3(a.y, b.y, c.y);
    int xmax = max3(a.x, b.x, c.x);
    int ymax = max3(a.y, b.y, c.y);

    if (((xmax - xmin) > 2048) || ((ymax - ymin) > 1024))
        return;

    float area = EDGE(a, b, c);

    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            int bc = (x >= gpu->draw_x1) && (x <= gpu->draw_x2) &&
                     (y >= gpu->draw_y1) && (y <= gpu->draw_y2);

            if (!bc)
                continue;

            p.x = x;
            p.y = y;

            float z0 = EDGE(b, c, p);

            if (TL(z0, b, c))
                continue;

            float z1 = EDGE(c, a, p);

            if (TL(z1, c, a))
                continue;

            float z2 = EDGE(a, b, p);

            if (TL(z2, a, b))
                continue;

            uint16_t color = 0;
            uint32_t mod   = 0;

            if (data.attrib & PA_SHADED) {
                float cr = (z0 * ((a.c >>  0) & 0xff) + z1 * ((b.c >>  0) & 0xff) + z2 * ((c.c >>  0) & 0xff)) / area;
                float cg = (z0 * ((a.c >>  8) & 0xff) + z1 * ((b.c >>  8) & 0xff) + z2 * ((c.c >>  8) & 0xff)) / area;
                float cb = (z0 * ((a.c >> 16) & 0xff) + z1 * ((b.c >> 16) & 0xff) + z2 * ((c.c >> 16) & 0xff)) / area;

                int dy = (y - ymin) & 3;
                int dx = (x - xmin) & 3;

                int dither = g_psx_gpu_dither_kernel[dx + (dy * 4)];

                cr += dither;
                cg += dither;
                cb += dither;

                // Saturate (clamp) to 00-ff
                cr = (cr >= 255.0f) ? 255.0f : ((cr <= 0.0f) ? 0.0f : cr);
                cg = (cg >= 255.0f) ? 255.0f : ((cg <= 0.0f) ? 0.0f : cg);
                cb = (cb >= 255.0f) ? 255.0f : ((cb <= 0.0f) ? 0.0f : cb);

                unsigned int ucr = roundf(cr);
                unsigned int ucg = roundf(cg);
                unsigned int ucb = roundf(cb);

                uint32_t rgb = (ucb << 16) | (ucg << 8) | ucr;

                mod = rgb;
            } else {
                mod = data.v[0].c;
            }

            if (data.attrib & PA_TEXTURED) {
                float tx = ((z0 * a.tx) + (z1 * b.tx) + (z2 * c.tx)) / area;
                float ty = ((z0 * a.ty) + (z1 * b.ty) + (z2 * c.ty)) / area;

                uint16_t texel = gpu_fetch_texel_bilinear(gpu, tx, ty, tpx, tpy, clutx, cluty, depth);

                if (!texel)
                    continue;

                if (data.attrib & PA_TRANSP)
                    transp = (texel & 0x8000) != 0;

                if (data.attrib & PA_RAW) {
                    color = texel;
                } else {
                    float tr = ((texel >> 0 ) & 0x1f) << 3;
                    float tg = ((texel >> 5 ) & 0x1f) << 3;
                    float tb = ((texel >> 10) & 0x1f) << 3;

                    float mr = (mod >> 0 ) & 0xff;
                    float mg = (mod >> 8 ) & 0xff;
                    float mb = (mod >> 16) & 0xff;

                    float cr = (tr * mr) / 128.0f;
                    float cg = (tg * mg) / 128.0f;
                    float cb = (tb * mb) / 128.0f;

                    cr = (cr >= 255.0f) ? 255.0f : ((cr <= 0.0f) ? 0.0f : cr);
                    cg = (cg >= 255.0f) ? 255.0f : ((cg <= 0.0f) ? 0.0f : cg);
                    cb = (cb >= 255.0f) ? 255.0f : ((cb <= 0.0f) ? 0.0f : cb);

                    unsigned int ucr = roundf(cr);
                    unsigned int ucg = roundf(cg);
                    unsigned int ucb = roundf(cb);

                    uint32_t rgb = ucr | (ucg << 8) | (ucb << 16);

                    color = BGR555(rgb);
                }
            } else {
                color = BGR555(mod);
            }

            float cr = ((color >> 0 ) & 0x1f) << 3;
            float cg = ((color >> 5 ) & 0x1f) << 3;
            float cb = ((color >> 10) & 0x1f) << 3;

            if (transp) {
                uint16_t back = gpu->vram[x + (y * 1024)];

                float br = ((back >> 0 ) & 0x1f) << 3;
                float bg = ((back >> 5 ) & 0x1f) << 3;
                float bb = ((back >> 10) & 0x1f) << 3;

                // Do we use transp or gpustat here?
                switch (transp_mode) {
                    case 0: {
                        cr = (0.5f * br) + (0.5f * cr);
                        cg = (0.5f * bg) + (0.5f * cg);
                        cb = (0.5f * bb) + (0.5f * cb);
                    } break;
                    case 1: {
                        cr = br + cr;
                        cg = bg + cg;
                        cb = bb + cb;
                    } break;
                    case 2: {
                        cr = br - cr;
                        cg = bg - cg;
                        cb = bb - cb;
                    } break;
                    case 3: {
                        cr = br + (0.25 * cr);
                        cg = bg + (0.25 * cg);
                        cb = bb + (0.25 * cb);
                    } break;
                }

                cr = (cr >= 255.0f) ? 255.0f : ((cr <= 0.0f) ? 0.0f : cr);
                cg = (cg >= 255.0f) ? 255.0f : ((cg <= 0.0f) ? 0.0f : cg);
                cb = (cb >= 255.0f) ? 255.0f : ((cb <= 0.0f) ? 0.0f : cb);

                unsigned int ucr = roundf(cr);
                unsigned int ucg = roundf(cg);
                unsigned int ucb = roundf(cb);

                uint32_t rgb = ucr | (ucg << 8) | (ucb << 16);

                color = BGR555(rgb);
            }

            gpu->vram[x + (y * 1024)] = color;
        }
    }
}

#define CLAMP(v, d, u) ((v) <= (d)) ? (d) : (((v) >= (u)) ? (u) : (v))

void gpu_render_rect(psx_gpu_t* gpu, rect_data_t data) {
    if ((data.v0.x >= 1024) || (data.v0.y >= 512))
        return;

    if ((data.v0.x <= -1024) || (data.v0.y <= -512))
        return;

    uint16_t width, height;

    switch ((data.attrib >> 3) & 3) {
        case RS_VARIABLE: { width = data.width; height = data.height; } break;
        case RS_1X1     : { width = 1         ; height = 1          ; } break;
        case RS_8X8     : { width = 8         ; height = 8          ; } break;
        case RS_16X16   : { width = 16        ; height = 16         ; } break;
    }

    int textured = (data.attrib & RA_TEXTURED) != 0;
    int transp = (data.attrib & RA_TRANSP) != 0;
    int transp_mode = (gpu->gpustat >> 5) & 3;

    int clutx = (data.clut & 0x3f) << 4;
    int cluty = (data.clut >> 6) & 0x1ff;

    /* Offset coordinates */
    data.v0.x += gpu->off_x;
    data.v0.y += gpu->off_y;

    /* Calculate bounding box */
    int xmax = data.v0.x + width;
    int ymax = data.v0.y + height;

    xmax = CLAMP(xmax, -1024, 1024);
    ymax = CLAMP(ymax, -1024, 1024);
    data.v0.x = CLAMP(data.v0.x, -1024, 1024);
    data.v0.y = CLAMP(data.v0.y, -1024, 1024);

    int32_t xc = 0, yc = 0;

    for (int16_t y = data.v0.y; y < ymax; y++) {
        for (int16_t x = data.v0.x; x < xmax; x++) {
            int bc = (x >= gpu->draw_x1) && (x <= gpu->draw_x2) &&
                     (y >= gpu->draw_y1) && (y <= gpu->draw_y2);

            if (!bc)
                goto skip;

            uint16_t color;

            if (textured) {
                uint16_t texel = gpu_fetch_texel(
                    gpu,
                    data.v0.tx + xc, data.v0.ty + yc,
                    gpu->texp_x, gpu->texp_y,
                    clutx, cluty,
                    gpu->texp_d
                );

                if (!texel)
                    goto skip;

                if ((data.attrib & RA_TRANSP) != 0)
                    transp = (texel & 0x8000) != 0;

                float tr = ((texel >> 0 ) & 0x1f) << 3;
                float tg = ((texel >> 5 ) & 0x1f) << 3;
                float tb = ((texel >> 10) & 0x1f) << 3;

                float mr = (data.v0.c >> 0 ) & 0xff;
                float mg = (data.v0.c >> 8 ) & 0xff;
                float mb = (data.v0.c >> 16) & 0xff;

                float cr = (tr * mr) / 128.0f;
                float cg = (tg * mg) / 128.0f;
                float cb = (tb * mb) / 128.0f;

                cr = (cr >= 255.0f) ? 255.0f : ((cr <= 0.0f) ? 0.0f : cr);
                cg = (cg >= 255.0f) ? 255.0f : ((cg <= 0.0f) ? 0.0f : cg);
                cb = (cb >= 255.0f) ? 255.0f : ((cb <= 0.0f) ? 0.0f : cb);

                unsigned int ucr = roundf(cr);
                unsigned int ucg = roundf(cg);
                unsigned int ucb = roundf(cb);

                uint32_t rgb = ucr | (ucg << 8) | (ucb << 16);

                color = BGR555(rgb);
            } else {
                color = BGR555(data.v0.c);
            }

            float cr = ((color >> 0 ) & 0x1f) << 3;
            float cg = ((color >> 5 ) & 0x1f) << 3;
            float cb = ((color >> 10) & 0x1f) << 3;

            if (transp) {
                uint16_t back = gpu->vram[x + (y * 1024)];

                float br = ((back >> 0 ) & 0x1f) << 3;
                float bg = ((back >> 5 ) & 0x1f) << 3;
                float bb = ((back >> 10) & 0x1f) << 3;

                switch (transp_mode) {
                    case 0: {
                        cr = (0.5f * br) + (0.5f * cr);
                        cg = (0.5f * bg) + (0.5f * cg);
                        cb = (0.5f * bb) + (0.5f * cb);
                    } break;
                    case 1: {
                        cr = br + cr;
                        cg = bg + cg;
                        cb = bb + cb;
                    } break;
                    case 2: {
                        cr = br - cr;
                        cg = bg - cg;
                        cb = bb - cb;
                    } break;
                    case 3: {
                        cr = br + (0.25f * cr);
                        cg = bg + (0.25f * cg);
                        cb = bb + (0.25f * cb);
                    } break;
                }

                cr = (cr >= 255.0f) ? 255.0f : ((cr <= 0.0f) ? 0.0f : cr);
                cg = (cg >= 255.0f) ? 255.0f : ((cg <= 0.0f) ? 0.0f : cg);
                cb = (cb >= 255.0f) ? 255.0f : ((cb <= 0.0f) ? 0.0f : cb);

                unsigned int ucr = roundf(cr);
                unsigned int ucg = roundf(cg);
                unsigned int ucb = roundf(cb);

                uint32_t rgb = ucr | (ucg << 8) | (ucb << 16);

                color = BGR555(rgb);
            }

            gpu->vram[x + (y * 1024)] = color;

            skip:

            ++xc;
        }

        xc = 0;

        ++yc;
    }
}

void plotLineLow(psx_gpu_t* gpu, int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int d = (2 * dy) - dx;
    int y = y0;

    for (int x = x0; x < x1; x++) {
        int bc = (x >= gpu->draw_x1) && (x <= gpu->draw_x2) &&
                 (y >= gpu->draw_y1) && (y <= gpu->draw_y2);

        if ((x < 1024) && (y < 512) && (x >= 0) && (y >= 0) && bc)
            gpu->vram[x + (y * 1024)] = color;

        if (d > 0) {
            y += yi;
            d += (2 * (dy - dx));
        } else {
            d += 2*dy;
        }
    }
}

void plotLineHigh(psx_gpu_t* gpu, int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }
    int d = (2 * dx) - dy;
    int x = x0;

    for (int y = y0; y < y1; y++) {
        int bc = (x >= gpu->draw_x1) && (x <= gpu->draw_x2) &&
                 (y >= gpu->draw_y1) && (y <= gpu->draw_y2);

        if ((x < 1024) && (y < 512) && (x >= 0) && (y >= 0) && bc)
            gpu->vram[x + (y * 1024)] = color;

        if (d > 0) {
            x = x + xi;
            d += (2 * (dx - dy));
        } else {
            d += 2*dx;
        }
    }
}

void plotLine(psx_gpu_t* gpu, int x0, int y0, int x1, int y1, uint16_t color) {
    if (abs(y1 - y0) < abs(x1 - x0)) {
        if (x0 > x1) {
            plotLineLow(gpu, x1, y1, x0, y0, color);
        } else {
            plotLineLow(gpu, x0, y0, x1, y1, color);
        }
    } else {
        if (y0 > y1) {
            plotLineHigh(gpu, x1, y1, x0, y0, color);
        } else {
            plotLineHigh(gpu, x0, y0, x1, y1, color);
        }
    }
}

void gpu_render_flat_line(psx_gpu_t* gpu, vertex_t v0, vertex_t v1, uint32_t color) {
    v0.x += gpu->off_x;
    v0.y += gpu->off_y;
    v1.x += gpu->off_x;
    v1.y += gpu->off_y;
    
    plotLine(gpu, v0.x, v0.y, v1.x, v1.y, color);
}

void gpu_render_flat_rectangle(psx_gpu_t* gpu, vertex_t v, uint32_t w, uint32_t h, uint32_t color) {
    /* Offset coordinates */
    v.x += gpu->off_x;
    v.y += gpu->off_y;

    /* Calculate bounding box */
    int xmin = max(v.x, gpu->draw_x1);
    int ymin = max(v.y, gpu->draw_y1);
    int xmax = min(xmin + w, gpu->draw_x2);
    int ymax = min(ymin + h, gpu->draw_y2);

    for (uint32_t y = ymin; y < ymax; y++) {
        for (uint32_t x = xmin; x < xmax; x++) {
            int bc = (x >= gpu->draw_x1) && (x <= gpu->draw_x2) &&
                     (y >= gpu->draw_y1) && (y <= gpu->draw_y2);

            if (!bc)
                continue;

            gpu->vram[x + (y * 1024)] = color;
        }
    }
}

void gpu_render_textured_rectangle(psx_gpu_t* gpu, vertex_t v, uint32_t w, uint32_t h, uint16_t clutx, uint16_t cluty, uint32_t color) {
    vertex_t a = v;

    a.x += gpu->off_x;
    a.y += gpu->off_y;

    int xmin = max(a.x, gpu->draw_x1);
    int ymin = max(a.y, gpu->draw_y1);
    int xmax = min(xmin + w, gpu->draw_x2);
    int ymax = min(ymin + h, gpu->draw_y2);

    uint32_t xc = 0, yc = 0;

    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            uint16_t texel = gpu_fetch_texel(
                gpu,
                a.tx + xc, a.ty + yc,
                gpu->texp_x, gpu->texp_y,
                clutx, cluty,
                gpu->texp_d
            );

            ++xc;

            gpu->vram[x + (y * 1024)] = texel;
        }

        xc = 0;

        ++yc;
    }
}

void gpu_render_flat_triangle(psx_gpu_t* gpu, vertex_t v0, vertex_t v1, vertex_t v2, uint32_t color) {
    vertex_t a, b, c;

    a = v0;

    /* Ensure the winding order is correct */
    if (EDGE(v0, v1, v2) < 0) {
        b = v2;
        c = v1;
    } else {
        b = v1;
        c = v2;
    }

    a.x += gpu->off_x;
    a.y += gpu->off_y;
    b.x += gpu->off_x;
    b.y += gpu->off_y;
    c.x += gpu->off_x;
    c.y += gpu->off_y;

    int xmin = max(min(min(a.x, b.x), c.x), gpu->draw_x1);
    int ymin = max(min(min(a.y, b.y), c.y), gpu->draw_y1);
    int xmax = min(max(max(a.x, b.x), c.x), gpu->draw_x2); 
    int ymax = min(max(max(a.y, b.y), c.y), gpu->draw_y2);

    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            int z0 = ((b.x - a.x) * (y - a.y)) - ((b.y - a.y) * (x - a.x));
            int z1 = ((c.x - b.x) * (y - b.y)) - ((c.y - b.y) * (x - b.x));
            int z2 = ((a.x - c.x) * (y - c.y)) - ((a.y - c.y) * (x - c.x));

            if ((z0 >= 0) && (z1 >= 0) && (z2 >= 0)) {
                gpu->vram[x + (y * 1024)] = BGR555(color);
            }
        }
    }
}

void gpu_render_shaded_triangle(psx_gpu_t* gpu, vertex_t v0, vertex_t v1, vertex_t v2) {
    vertex_t a, b, c, p;

    a = v0;

    /* Ensure the winding order is correct */
    if (EDGE(v0, v1, v2) < 0) {
        b = v2;
        c = v1;
    } else {
        b = v1;
        c = v2;
    }

    a.x += gpu->off_x;
    a.y += gpu->off_y;
    b.x += gpu->off_x;
    b.y += gpu->off_y;
    c.x += gpu->off_x;
    c.y += gpu->off_y;

    int xmin = max(min(min(a.x, b.x), c.x), gpu->draw_x1);
    int ymin = max(min(min(a.y, b.y), c.y), gpu->draw_y1);
    int xmax = min(max(max(a.x, b.x), c.x), gpu->draw_x2); 
    int ymax = min(max(max(a.y, b.y), c.y), gpu->draw_y2);

    int area = EDGE(a, b, c);

    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            p.x = x;
            p.y = y;

            float z0 = EDGE((float)b, (float)c, (float)p);
            float z1 = EDGE((float)c, (float)a, (float)p);
            float z2 = EDGE((float)a, (float)b, (float)p);

            if ((z0 >= 0) && (z1 >= 0) && (z2 >= 0)) {
                int cr = (z0 * ((a.c >>  0) & 0xff) + z1 * ((b.c >>  0) & 0xff) + z2 * ((c.c >>  0) & 0xff)) / area;
                int cg = (z0 * ((a.c >>  8) & 0xff) + z1 * ((b.c >>  8) & 0xff) + z2 * ((c.c >>  8) & 0xff)) / area;
                int cb = (z0 * ((a.c >> 16) & 0xff) + z1 * ((b.c >> 16) & 0xff) + z2 * ((c.c >> 16) & 0xff)) / area;

                // Calculate positions within our 4x4 dither
                // kernel
                int dy = (y - ymin) % 4;
                int dx = (x - xmin) % 4;

                // Shift two pixels horizontally on the last
                // two scanlines?
                // if (dy > 1) {
                //     dx = ((x + 2) - xmin) % 4;
                // }

                int dither = g_psx_gpu_dither_kernel[dx + (dy * 4)];

                // Add to the original 8-bit color values
                cr += dither;
                cg += dither;
                cb += dither;

                // Saturate (clamp) to 00-ff
                cr = (cr >= 0xff) ? 0xff : ((cr <= 0) ? 0 : cr);
                cg = (cg >= 0xff) ? 0xff : ((cg <= 0) ? 0 : cg);
                cb = (cb >= 0xff) ? 0xff : ((cb <= 0) ? 0 : cb);

                uint32_t color = (cb << 16) | (cg << 8) | cr;

                gpu->vram[x + (y * 1024)] = BGR555(color);
            }
        }
    }
}

void gpu_render_textured_triangle(psx_gpu_t* gpu, vertex_t v0, vertex_t v1, vertex_t v2, uint32_t tpx, uint32_t tpy, uint16_t clutx, uint16_t cluty, int depth) {
    vertex_t a, b, c;

    a = v0;

    /* Ensure the winding order is correct */
    if (EDGE(v0, v1, v2) < 0) {
        b = v2;
        c = v1;
    } else {
        b = v1;
        c = v2;
    }

    a.x += gpu->off_x;
    a.y += gpu->off_y;
    b.x += gpu->off_x;
    b.y += gpu->off_y;
    c.x += gpu->off_x;
    c.y += gpu->off_y;

    int xmin = max(min(min(a.x, b.x), c.x), gpu->draw_x1);
    int ymin = max(min(min(a.y, b.y), c.y), gpu->draw_y1);
    int xmax = min(max(max(a.x, b.x), c.x), gpu->draw_x2); 
    int ymax = min(max(max(a.y, b.y), c.y), gpu->draw_y2);

    uint32_t area = EDGE(a, b, c);

    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            vertex_t p;

            p.x = x;
            p.y = y;

            float z0 = EDGE((float)b, (float)c, (float)p);
            float z1 = EDGE((float)c, (float)a, (float)p);
            float z2 = EDGE((float)a, (float)b, (float)p);

            if ((z0 >= 0) && (z1 >= 0) && (z2 >= 0)) {
                uint32_t tx = ((z0 * a.tx) + (z1 * b.tx) + (z2 * c.tx)) / area;
                uint32_t ty = ((z0 * a.ty) + (z1 * b.ty) + (z2 * c.ty)) / area;

                uint16_t color = gpu_fetch_texel(
                    gpu,
                    tx, ty,
                    tpx, tpy,
                    clutx, cluty,
                    depth
                );

                if (!color) continue;

                gpu->vram[x + (y * 1024)] = color;
            }
        }
    }
}

#define I32(v, b) (((int32_t)((v) << (31-b))) >> (31-b))

void gpu_rect(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;

            int size = (gpu->buf[0] >> 27) & 3;
            int textured = (gpu->buf[0] & 0x04000000) != 0;

            gpu->cmd_args_remaining = 1 + (size == RS_VARIABLE) + textured;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                rect_data_t rect;

                rect.attrib = gpu->buf[0] >> 24;

                int textured = (rect.attrib & RA_TEXTURED) != 0;
                int raw      = (rect.attrib & RA_RAW) != 0;

                // Add 1 if is textured
                int size_offset = 2 + textured;

                rect.v0.c   = gpu->buf[0] & 0xffffff;
                rect.v0.x   = SE10(gpu->buf[1] & 0xffff);
                rect.v0.y   = SE10(gpu->buf[1] >> 16);
                rect.v0.tx  = (gpu->buf[2] >> 0) & 0xff;
                rect.v0.ty  = (gpu->buf[2] >> 8) & 0xff;
                rect.clut   = gpu->buf[2] >> 16;
                rect.width  = gpu->buf[size_offset] & 0xffff;
                rect.height = gpu->buf[size_offset] >> 16;

                if (textured && raw)
                    rect.v0.c = 0x808080;

                gpu_render_rect(gpu, rect);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_poly(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;

            int shaded   = (gpu->buf[0] & 0x10000000) != 0;
            int quad     = (gpu->buf[0] & 0x08000000) != 0;
            int textured = (gpu->buf[0] & 0x04000000) != 0;

            int fields_per_vertex = 1 + shaded + textured;
            int vertices = 3 + quad;
 
            gpu->cmd_args_remaining = (fields_per_vertex * vertices) - shaded;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                poly_data_t poly;

                poly.attrib = gpu->buf[0] >> 24;

                int shaded   = (poly.attrib & PA_SHADED) != 0;
                int textured = (poly.attrib & PA_TEXTURED) != 0;

                int color_offset = shaded * (2 + textured);
                int vert_offset = 1 + (textured | shaded) +
                                      (textured & shaded);
                int texc_offset = textured * (2 + shaded);
                int texp_offset = textured * (4 + shaded);

                poly.clut = gpu->buf[2] >> 16;
                poly.texp = gpu->buf[texp_offset] >> 16;

                // Undocumented behavior?
                // Fixes Mortal Kombat II, Bubble Bobble, Driver 1 & 2
                if (textured) {
                    gpu->texp_x = (poly.texp & 0xf) << 6;
                    gpu->texp_y = (poly.texp & 0x10) << 4;
                    gpu->texp_d = (poly.texp >> 7) & 0x3;
                    gpu->gpustat &= 0xfffffe00;
                    gpu->gpustat |= poly.texp & 0x1ff;
                }

                poly.v[0].c = gpu->buf[0+0*color_offset] & 0xffffff;
                poly.v[1].c = gpu->buf[0+1*color_offset] & 0xffffff;
                poly.v[2].c = gpu->buf[0+2*color_offset] & 0xffffff;
                poly.v[3].c = gpu->buf[0+3*color_offset] & 0xffffff;
                poly.v[0].x = SE10(gpu->buf[1+0*vert_offset] & 0xffff);
                poly.v[1].x = SE10(gpu->buf[1+1*vert_offset] & 0xffff);
                poly.v[2].x = SE10(gpu->buf[1+2*vert_offset] & 0xffff);
                poly.v[3].x = SE10(gpu->buf[1+3*vert_offset] & 0xffff);
                poly.v[0].y = SE10(gpu->buf[1+0*vert_offset] >> 16);
                poly.v[1].y = SE10(gpu->buf[1+1*vert_offset] >> 16);
                poly.v[2].y = SE10(gpu->buf[1+2*vert_offset] >> 16);
                poly.v[3].y = SE10(gpu->buf[1+3*vert_offset] >> 16);
                poly.v[0].tx = gpu->buf[2+0*texc_offset] & 0xff;
                poly.v[1].tx = gpu->buf[2+1*texc_offset] & 0xff;
                poly.v[2].tx = gpu->buf[2+2*texc_offset] & 0xff;
                poly.v[3].tx = gpu->buf[2+3*texc_offset] & 0xff;
                poly.v[0].ty = (gpu->buf[2+0*texc_offset] >> 8) & 0xff;
                poly.v[1].ty = (gpu->buf[2+1*texc_offset] >> 8) & 0xff;
                poly.v[2].ty = (gpu->buf[2+2*texc_offset] >> 8) & 0xff;
                poly.v[3].ty = (gpu->buf[2+3*texc_offset] >> 8) & 0xff;

                if (poly.attrib & PA_QUAD) {
                    gpu_render_triangle(gpu, poly.v[0], poly.v[1], poly.v[2], poly, 1);
                    gpu_render_triangle(gpu, poly.v[1], poly.v[2], poly.v[3], poly, 1);
                } else {
                    gpu_render_triangle(gpu, poly.v[0], poly.v[1], poly.v[2], poly, 0);
                }

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_line(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;

            int shaded   = (gpu->buf[0] & 0x10000000) != 0;
            int polyline = (gpu->buf[0] & 0x08000000) != 0;

            gpu->cmd_args_remaining = polyline ? -1 : (shaded ? 3 : 2);
            gpu->line_done = 0;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (gpu->buf[0] & 0x08000000) {
                if ((gpu->buf[gpu->buf_index - 1] & 0xf000f000) == 0x50005000) {
                    gpu->state = GPU_STATE_RECV_CMD;

                    return;
                }

                // int shaded = (gpu->buf[0] & 0x10000000) != 0;

                // if (shaded) {
                //     if (gpu->buf_index > 2) {

                //     }
                // }

                // if (gpu->buf_index > overflow) {
                //     vertex_t v0, v1;

                //     if (shaded) {
                //         v0.c = gpu->buf[0] & 0xffffff;
                //         v1.c = gpu->buf[4] & 0xffffff;
                //         v0.x = gpu->buf[1] & 0xffff;
                //         v0.y = gpu->buf[1] >> 16;
                //         v1.x = gpu->buf[3] & 0xffff;
                //         v1.y = gpu->buf[3] >> 16;
                //     } else {
                //         v0.c = gpu->buf[0] & 0xffffff;
                //         v1.c = gpu->buf[0] & 0xffffff;
                //         v0.x = gpu->buf[1] & 0xffff;
                //         v0.y = gpu->buf[1] >> 16;
                //         v1.x = gpu->buf[2] & 0xffff;
                //         v1.y = gpu->buf[2] >> 16;
                //     }

                //     gpu->prev_line_vertex = v1;

                //     gpu_render_flat_line(gpu, v0, v1, gpu->buf[0] & 0xffffff);

                //     gpu->buf_index = 1;
                // }
            } else if (!gpu->cmd_args_remaining) {
                vertex_t v0, v1;

                if (gpu->buf[0] & 0x10000000) {
                    v0.c = gpu->buf[0] & 0xffffff;
                    v1.c = gpu->buf[2] & 0xffffff;
                    v0.x = gpu->buf[1] & 0xffff;
                    v0.y = gpu->buf[1] >> 16;
                    v1.x = gpu->buf[3] & 0xffff;
                    v1.y = gpu->buf[3] >> 16;
                } else {
                    v0.c = gpu->buf[0] & 0xffffff;
                    v1.c = gpu->buf[0] & 0xffffff;
                    v0.x = gpu->buf[1] & 0xffff;
                    v0.y = gpu->buf[1] >> 16;
                    v1.x = gpu->buf[2] & 0xffff;
                    v1.y = gpu->buf[2] >> 16;
                }

                gpu_render_flat_line(gpu, v0, v1, BGR555(gpu->buf[0] & 0xffffff));

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_a0(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                // Save static data
                gpu->xpos = gpu->buf[1] & 0x3ff;
                gpu->ypos = (gpu->buf[1] >> 16) & 0x1ff;
                gpu->xsiz = gpu->buf[2] & 0xffff;
                gpu->ysiz = gpu->buf[2] >> 16;
                gpu->xsiz = ((gpu->xsiz - 1) & 0x3ff) + 1;
                gpu->ysiz = ((gpu->ysiz - 1) & 0x1ff) + 1;
                gpu->tsiz = ((gpu->xsiz * gpu->ysiz) + 1) & 0xfffffffe;
                gpu->addr = gpu->xpos + (gpu->ypos * 1024);
                gpu->xcnt = 0;
                gpu->ycnt = 0;
            }
        } break;

        case GPU_STATE_RECV_DATA: {
            unsigned int xpos = (gpu->xpos + gpu->xcnt) & 0x3ff;
            unsigned int ypos = (gpu->ypos + gpu->ycnt) & 0x1ff;

            gpu->vram[xpos + (ypos * 1024)] = gpu->recv_data & 0xffff;

            ++gpu->xcnt;

            xpos = (gpu->xpos + gpu->xcnt) & 0x3ff;
            ypos = (gpu->ypos + gpu->ycnt) & 0x1ff;

            if (gpu->xcnt == gpu->xsiz) {
                ++gpu->ycnt;
                gpu->xcnt = 0;

                ypos = (gpu->ypos + gpu->ycnt) & 0x1ff;
                xpos = (gpu->xpos + gpu->xcnt) & 0x3ff;
            }

            gpu->vram[xpos + (ypos * 1024)] = gpu->recv_data >> 16;

            ++gpu->xcnt;
            
            if (gpu->xcnt == gpu->xsiz) {
                ++gpu->ycnt;
                gpu->xcnt = 0;

                xpos = (gpu->xpos + gpu->xcnt) & 0x3ff;
                ypos = (gpu->ypos + gpu->ycnt) & 0x1ff;
            }

            gpu->tsiz -= 2;

            if (!gpu->tsiz) {
                gpu->xcnt = 0;
                gpu->ycnt = 0;
                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

// Monochrome Opaque Quadrilateral
void gpu_cmd_28(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 4;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->v0.x = gpu->buf[1] & 0xffff;
                gpu->v0.y = gpu->buf[1] >> 16;
                gpu->v1.x = gpu->buf[2] & 0xffff;
                gpu->v1.y = gpu->buf[2] >> 16;
                gpu->v2.x = gpu->buf[3] & 0xffff;
                gpu->v2.y = gpu->buf[3] >> 16;
                gpu->v3.x = gpu->buf[4] & 0xffff;
                gpu->v3.y = gpu->buf[4] >> 16;
                gpu->color = gpu->buf[0] & 0xffffff;

                gpu_render_flat_triangle(gpu, gpu->v0, gpu->v1, gpu->v2, gpu->color);
                gpu_render_flat_triangle(gpu, gpu->v1, gpu->v2, gpu->v3, gpu->color);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

// Monochrome Opaque Quadrilateral
void gpu_cmd_30(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 5;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->v0.c = gpu->buf[0] & 0xffffff;
                gpu->v0.x = gpu->buf[1] & 0xffff;
                gpu->v0.y = gpu->buf[1] >> 16;
                gpu->v1.c = gpu->buf[2] & 0xffffff;
                gpu->v1.x = gpu->buf[3] & 0xffff;
                gpu->v1.y = gpu->buf[3] >> 16;
                gpu->v2.c = gpu->buf[4] & 0xffffff;
                gpu->v2.x = gpu->buf[5] & 0xffff;
                gpu->v2.y = gpu->buf[5] >> 16;

                gpu_render_shaded_triangle(gpu, gpu->v0, gpu->v1, gpu->v2);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

// Monochrome Opaque Quadrilateral
void gpu_cmd_38(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 7;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->v0.c = gpu->buf[0] & 0xffffff;
                gpu->v0.x = gpu->buf[1] & 0xffff;
                gpu->v0.y = gpu->buf[1] >> 16;
                gpu->v1.c = gpu->buf[2] & 0xffffff;
                gpu->v1.x = gpu->buf[3] & 0xffff;
                gpu->v1.y = gpu->buf[3] >> 16;
                gpu->v2.c = gpu->buf[4] & 0xffffff;
                gpu->v2.x = gpu->buf[5] & 0xffff;
                gpu->v2.y = gpu->buf[5] >> 16;
                gpu->v3.c = gpu->buf[6] & 0xffffff;
                gpu->v3.x = gpu->buf[7] & 0xffff;
                gpu->v3.y = gpu->buf[7] >> 16;

                gpu_render_shaded_triangle(gpu, gpu->v0, gpu->v1, gpu->v2);
                gpu_render_shaded_triangle(gpu, gpu->v1, gpu->v2, gpu->v3);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

// Monochrome Opaque Quadrilateral
void gpu_cmd_3c(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 11;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                uint32_t texp = gpu->buf[5] >> 16;
                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->pal   = gpu->buf[2] >> 16;
                gpu->v0.tx = gpu->buf[2] & 0xff;
                gpu->v0.ty = (gpu->buf[2] >> 8) & 0xff;
                gpu->v1.tx = gpu->buf[5] & 0xff;
                gpu->v1.ty = (gpu->buf[5] >> 8) & 0xff;
                gpu->v2.tx = gpu->buf[8] & 0xff;
                gpu->v2.ty = (gpu->buf[8] >> 8) & 0xff;
                gpu->v3.tx = gpu->buf[11] & 0xff;
                gpu->v3.ty = (gpu->buf[11] >> 8) & 0xff;
                gpu->v0.x = gpu->buf[1] & 0xffff;
                gpu->v0.y = gpu->buf[1] >> 16;
                gpu->v1.x = gpu->buf[4] & 0xffff;
                gpu->v1.y = gpu->buf[4] >> 16;
                gpu->v2.x = gpu->buf[7] & 0xffff;
                gpu->v2.y = gpu->buf[7] >> 16;
                gpu->v3.x = gpu->buf[10] & 0xffff;
                gpu->v3.y = gpu->buf[10] >> 16;

                uint16_t clutx = (gpu->pal & 0x3f) << 4;
                uint16_t cluty = (gpu->pal >> 6) & 0x1ff;
                uint16_t tpx = (texp & 0xf) << 6;
                uint16_t tpy = (texp & 0x10) << 4;
                uint16_t depth = (texp >> 7) & 0x3;

                gpu_render_textured_triangle(gpu, gpu->v0, gpu->v1, gpu->v2, tpx, tpy, clutx, cluty, depth);
                gpu_render_textured_triangle(gpu, gpu->v1, gpu->v2, gpu->v3, tpx, tpy, clutx, cluty, depth);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

// Monochrome Opaque Quadrilateral
void gpu_cmd_2c(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 8;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                uint32_t texp = gpu->buf[4] >> 16;
                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->pal   = gpu->buf[2] >> 16;
                gpu->v0.tx = gpu->buf[2] & 0xff;
                gpu->v0.ty = (gpu->buf[2] >> 8) & 0xff;
                gpu->v1.tx = gpu->buf[4] & 0xff;
                gpu->v1.ty = (gpu->buf[4] >> 8) & 0xff;
                gpu->v2.tx = gpu->buf[6] & 0xff;
                gpu->v2.ty = (gpu->buf[6] >> 8) & 0xff;
                gpu->v3.tx = gpu->buf[8] & 0xff;
                gpu->v3.ty = (gpu->buf[8] >> 8) & 0xff;
                gpu->v0.x = gpu->buf[1] & 0xffff;
                gpu->v0.y = gpu->buf[1] >> 16;
                gpu->v1.x = gpu->buf[3] & 0xffff;
                gpu->v1.y = gpu->buf[3] >> 16;
                gpu->v2.x = gpu->buf[5] & 0xffff;
                gpu->v2.y = gpu->buf[5] >> 16;
                gpu->v3.x = gpu->buf[7] & 0xffff;
                gpu->v3.y = gpu->buf[7] >> 16;

                uint16_t clutx = (gpu->pal & 0x3f) << 4;
                uint16_t cluty = (gpu->pal >> 6) & 0x1ff;
                uint16_t tpx = (texp & 0xf) << 6;
                uint16_t tpy = (texp & 0x10) << 4;
                uint16_t depth = (texp >> 7) & 0x3;

                gpu_render_textured_triangle(gpu, gpu->v0, gpu->v1, gpu->v2, tpx, tpy, clutx, cluty, depth);
                gpu_render_textured_triangle(gpu, gpu->v1, gpu->v2, gpu->v3, tpx, tpy, clutx, cluty, depth);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

// Monochrome Opaque Quadrilateral
void gpu_cmd_24(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 6;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                uint32_t texp = gpu->buf[4] >> 16;
                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->pal   = gpu->buf[2] >> 16;
                gpu->v0.tx = gpu->buf[2] & 0xff;
                gpu->v0.ty = (gpu->buf[2] >> 8) & 0xff;
                gpu->v1.tx = gpu->buf[4] & 0xff;
                gpu->v1.ty = (gpu->buf[4] >> 8) & 0xff;
                gpu->v2.tx = gpu->buf[6] & 0xff;
                gpu->v2.ty = (gpu->buf[6] >> 8) & 0xff;
                gpu->v0.x = gpu->buf[1] & 0xffff;
                gpu->v0.y = gpu->buf[1] >> 16;
                gpu->v1.x = gpu->buf[3] & 0xffff;
                gpu->v1.y = gpu->buf[3] >> 16;
                gpu->v2.x = gpu->buf[5] & 0xffff;
                gpu->v2.y = gpu->buf[5] >> 16;

                uint16_t clutx = (gpu->pal & 0x3f) << 4;
                uint16_t cluty = (gpu->pal >> 6) & 0x1ff;
                uint16_t tpx = (texp & 0xf) << 6;
                uint16_t tpy = (texp & 0x10) << 4;
                uint16_t depth = (texp >> 7) & 0x3;

                gpu_render_textured_triangle(gpu, gpu->v0, gpu->v1, gpu->v2, tpx, tpy, clutx, cluty, depth);
                gpu_render_textured_triangle(gpu, gpu->v1, gpu->v2, gpu->v3, tpx, tpy, clutx, cluty, depth);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

// Monochrome Opaque Quadrilateral
void gpu_cmd_2d(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 8;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                uint32_t texp = gpu->buf[4] >> 16;
                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->pal   = gpu->buf[2] >> 16;
                gpu->v0.tx = gpu->buf[2] & 0xff;
                gpu->v0.ty = (gpu->buf[2] >> 8) & 0xff;
                gpu->v1.tx = gpu->buf[4] & 0xff;
                gpu->v1.ty = (gpu->buf[4] >> 8) & 0xff;
                gpu->v2.tx = gpu->buf[6] & 0xff;
                gpu->v2.ty = (gpu->buf[6] >> 8) & 0xff;
                gpu->v3.tx = gpu->buf[8] & 0xff;
                gpu->v3.ty = (gpu->buf[8] >> 8) & 0xff;
                gpu->v0.x = gpu->buf[1] & 0xffff;
                gpu->v0.y = gpu->buf[1] >> 16;
                gpu->v1.x = gpu->buf[3] & 0xffff;
                gpu->v1.y = gpu->buf[3] >> 16;
                gpu->v2.x = gpu->buf[5] & 0xffff;
                gpu->v2.y = gpu->buf[5] >> 16;
                gpu->v3.x = gpu->buf[7] & 0xffff;
                gpu->v3.y = gpu->buf[7] >> 16;

                uint16_t clutx = (gpu->pal & 0x3f) << 4;
                uint16_t cluty = (gpu->pal >> 6) & 0x1ff;
                uint16_t tpx = (texp & 0xf) << 6;
                uint16_t tpy = (texp & 0x10) << 4;
                uint16_t depth = (texp >> 7) & 0x3;

                gpu_render_textured_triangle(gpu, gpu->v0, gpu->v1, gpu->v2, tpx, tpy, clutx, cluty, depth);
                gpu_render_textured_triangle(gpu, gpu->v1, gpu->v2, gpu->v3, tpx, tpy, clutx, cluty, depth);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_64(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 3;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->v0.x  = gpu->buf[1] & 0xffff;
                gpu->v0.y  = gpu->buf[1] >> 16;
                gpu->v0.tx = gpu->buf[2] & 0xff;
                gpu->v0.ty = (gpu->buf[2] >> 8) & 0xff;
                gpu->pal   = gpu->buf[2] >> 16;

                uint32_t w = gpu->buf[3] & 0xffff;
                uint32_t h = gpu->buf[3] >> 16;
                uint16_t clutx = (gpu->pal & 0x3f) << 4;
                uint16_t cluty = (gpu->pal >> 6) & 0x1ff;

                gpu_render_textured_rectangle(gpu, gpu->v0, w, h, clutx, cluty, gpu->color);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_7c(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->v0.x  = gpu->buf[1] & 0xffff;
                gpu->v0.y  = gpu->buf[1] >> 16;
                gpu->v0.tx = gpu->buf[2] & 0xff;
                gpu->v0.ty = (gpu->buf[2] >> 8) & 0xff;
                gpu->pal   = gpu->buf[2] >> 16;

                uint32_t w = 16;
                uint32_t h = 16;
                uint16_t clutx = (gpu->pal & 0x3f) << 4;
                uint16_t cluty = (gpu->pal >> 6) & 0x1ff;

                gpu_render_textured_rectangle(gpu, gpu->v0, w, h, clutx, cluty, gpu->color);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_74(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->v0.x  = gpu->buf[1] & 0xffff;
                gpu->v0.y  = gpu->buf[1] >> 16;
                gpu->v0.tx = gpu->buf[2] & 0xff;
                gpu->v0.ty = (gpu->buf[2] >> 8) & 0xff;
                gpu->pal   = gpu->buf[2] >> 16;

                uint32_t w = 8;
                uint32_t h = 8;
                uint16_t clutx = (gpu->pal & 0x3f) << 4;
                uint16_t cluty = (gpu->pal >> 6) & 0x1ff;

                gpu_render_textured_rectangle(gpu, gpu->v0, w, h, clutx, cluty, gpu->color);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_60(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->v0.x  = gpu->buf[1] & 0xffff;
                gpu->v0.y  = gpu->buf[1] >> 16;
                gpu->xsiz  = gpu->buf[2] & 0xffff;
                gpu->ysiz  = gpu->buf[2] >> 16;

                gpu->v0.x += gpu->off_x;
                gpu->v0.y += gpu->off_y;

                gpu_render_flat_rectangle(gpu, gpu->v0, gpu->xsiz, gpu->ysiz, BGR555(gpu->color));

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_68(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 1;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->v0.x  = gpu->buf[1] & 0xffff;
                gpu->v0.y  = gpu->buf[1] >> 16;

                gpu->v0.x += gpu->off_x;
                gpu->v0.y += gpu->off_y;

                gpu->vram[gpu->v0.x + (gpu->v0.y * 1024)] = BGR555(gpu->color);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_40(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->v0.x  = gpu->buf[1] & 0xffff;
                gpu->v0.y  = gpu->buf[1] >> 16;
                gpu->v1.x  = gpu->buf[2] & 0xffff;
                gpu->v1.y  = gpu->buf[2] >> 16;

                gpu_render_flat_line(gpu, gpu->v0, gpu->v1, BGR555(gpu->color));

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_c0(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->c0_xcnt = 0;
                gpu->c0_ycnt = 0;
                uint32_t c0_xpos = gpu->buf[1] & 0xffff;
                uint32_t c0_ypos = gpu->buf[1] >> 16;
                gpu->c0_xsiz = gpu->buf[2] & 0xffff;
                gpu->c0_ysiz = gpu->buf[2] >> 16;
                c0_xpos = c0_xpos & 0x3ff;
                c0_ypos = c0_ypos & 0x1ff;
                gpu->c0_xsiz = ((gpu->c0_xsiz - 1) & 0x3ff) + 1;
                gpu->c0_ysiz = ((gpu->c0_ysiz - 1) & 0x1ff) + 1;
                gpu->c0_tsiz = ((gpu->c0_xsiz * gpu->c0_ysiz) + 1) & 0xfffffffe;
                gpu->c0_addr = c0_xpos + (c0_ypos * 1024);

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_02(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 2;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                gpu->color = gpu->buf[0] & 0xffffff;
                gpu->v0.x  = gpu->buf[1] & 0xffff;
                gpu->v0.y  = gpu->buf[1] >> 16;
                gpu->xsiz  = gpu->buf[2] & 0xffff;
                gpu->ysiz  = gpu->buf[2] >> 16;

                gpu->v0.x = (gpu->v0.x & 0x3f0);
                gpu->v0.y = gpu->v0.y & 0x1ff;
                gpu->xsiz = (((gpu->xsiz & 0x3ff) + 0x0f) & 0xfffffff0);
                gpu->ysiz = gpu->ysiz & 0x1ff;

                uint16_t color = BGR555(gpu->color);

                // printf("02 draw=(%u,%u-%u,%u) v0=(%u,%u) siz=(%u,%u)\n",
                //     gpu->draw_x1,
                //     gpu->draw_y1,
                //     gpu->draw_x2,
                //     gpu->draw_y2,
                //     gpu->v0.x,
                //     gpu->v0.y,
                //     gpu->xsiz,
                //     gpu->ysiz
                // );

                for (int y = gpu->v0.y; y < (gpu->v0.y + gpu->ysiz); y++) {
                    for (int x = gpu->v0.x; x < (gpu->v0.x + gpu->xsiz); x++) {
                        // This shouldn't be needed
                        // int bc = (x >= gpu->draw_x1) && (x <= gpu->draw_x2) &&
                        //          (y >= gpu->draw_y1) && (y <= gpu->draw_y2);

                        // if (!bc)
                        //     continue;

                        if ((x < 1024) && (y < 512) && (x >= 0) && (y >= 0))
                            gpu->vram[x + (y * 1024)] = color;
                    }
                }

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void gpu_cmd_80(psx_gpu_t* gpu) {
    switch (gpu->state) {
        case GPU_STATE_RECV_CMD: {
            gpu->state = GPU_STATE_RECV_ARGS;
            gpu->cmd_args_remaining = 3;
        } break;

        case GPU_STATE_RECV_ARGS: {
            if (!gpu->cmd_args_remaining) {
                gpu->state = GPU_STATE_RECV_DATA;

                uint32_t srcx = gpu->buf[1] & 0xffff;
                uint32_t srcy = gpu->buf[1] >> 16;
                uint32_t dstx = gpu->buf[2] & 0xffff;
                uint32_t dsty = gpu->buf[2] >> 16;
                uint32_t xsiz = gpu->buf[3] & 0xffff;
                uint32_t ysiz = gpu->buf[3] >> 16;

                for (int y = 0; y < ysiz; y++) {
                    for (int x = 0; x < xsiz; x++) {
                        int dstb = ((dstx + x) < 1024) && ((dsty + y) < 512);
                        int srcb = ((srcx + x) < 1024) && ((srcy + y) < 512);
                        
                        if (dstb && srcb)
                            gpu->vram[(dstx + x) + (dsty + y) * 1024] = gpu->vram[(srcx + x) + (srcy + y) * 1024];
                    }
                }

                gpu->state = GPU_STATE_RECV_CMD;
            }
        } break;
    }
}

void psx_gpu_update_cmd(psx_gpu_t* gpu) {
    int type = (gpu->buf[0] >> 29) & 7;

    switch (type) {
        case 1: gpu_poly(gpu); return;
        case 2: gpu_line(gpu); return;
        case 3: gpu_rect(gpu); return;
    }

    switch (gpu->buf[0] >> 24) {
        case 0x00: /* nop */ break;
        case 0x01: /* Cache clear */ break;
        case 0x02: gpu_cmd_02(gpu); break;
        case 0x24: gpu_cmd_24(gpu); break;
        case 0x25: gpu_cmd_24(gpu); break;
        case 0x26: gpu_cmd_24(gpu); break;
        case 0x27: gpu_cmd_24(gpu); break;
        case 0x28: gpu_cmd_28(gpu); break;
        case 0x2a: gpu_cmd_28(gpu); break;
        case 0x2c: gpu_cmd_2d(gpu); break;
        case 0x2d: gpu_cmd_2d(gpu); break;
        case 0x2e: gpu_cmd_2d(gpu); break;
        case 0x2f: gpu_cmd_2d(gpu); break;
        case 0x30: gpu_cmd_30(gpu); break;
        case 0x32: gpu_cmd_30(gpu); break;
        case 0x38: gpu_cmd_38(gpu); break;
        case 0x3c: gpu_cmd_3c(gpu); break;
        case 0x3e: gpu_cmd_3c(gpu); break;
        case 0x40: gpu_cmd_40(gpu); break;
        case 0x60: gpu_cmd_60(gpu); break;
        case 0x62: gpu_cmd_60(gpu); break;
        case 0x64: gpu_cmd_64(gpu); break;
        case 0x65: gpu_cmd_64(gpu); break;
        case 0x66: gpu_cmd_64(gpu); break;
        case 0x67: gpu_cmd_64(gpu); break;
        case 0x68: gpu_cmd_68(gpu); break;
        case 0x74: gpu_cmd_74(gpu); break;
        case 0x75: gpu_cmd_74(gpu); break;
        case 0x76: gpu_cmd_74(gpu); break;
        case 0x77: gpu_cmd_74(gpu); break;
        case 0x7c: gpu_cmd_7c(gpu); break;
        case 0x7d: gpu_cmd_7c(gpu); break;
        case 0x7e: gpu_cmd_7c(gpu); break;
        case 0x7f: gpu_cmd_7c(gpu); break;
        case 0x80: gpu_cmd_80(gpu); break;
        case 0xa0: gpu_cmd_a0(gpu); break;
        case 0xc0: gpu_cmd_c0(gpu); break;
        case 0xe1: {
            gpu->gpustat &= 0xfffff800;
            gpu->gpustat |= gpu->buf[0] & 0x7ff;
            gpu->texp_x = (gpu->gpustat & 0xf) << 6;
            gpu->texp_y = (gpu->gpustat & 0x10) << 4;
            gpu->texp_d = (gpu->gpustat >> 7) & 0x3;
        } break;
        case 0xe2: {
            gpu->texw_mx = ((gpu->buf[0] >> 0 ) & 0x1f) << 3;
            gpu->texw_my = ((gpu->buf[0] >> 5 ) & 0x1f) << 3;
            gpu->texw_ox = ((gpu->buf[0] >> 10) & 0x1f) << 3;
            gpu->texw_oy = ((gpu->buf[0] >> 15) & 0x1f) << 3;
        } break;
        case 0xe3: {
            gpu->draw_x1 = (gpu->buf[0] >> 0 ) & 0x3ff;
            gpu->draw_y1 = (gpu->buf[0] >> 10) & 0x1ff;
        } break;
        case 0xe4: {
            gpu->draw_x2 = (gpu->buf[0] >> 0 ) & 0x3ff;
            gpu->draw_y2 = (gpu->buf[0] >> 10) & 0x1ff;
        } break;
        case 0xe5: {
            gpu->off_x = ((int32_t)(((gpu->buf[0] >> 0 ) & 0x7ff) << 21)) >> 21;
            gpu->off_y = ((int32_t)(((gpu->buf[0] >> 11) & 0x7ff) << 21)) >> 21;
        } break;
        case 0xe6: {
            /* To-do: Implement mask bit thing */
        } break;
        default: {
            // log_set_quiet(0);
            // log_fatal("Unhandled GP0(%02Xh)", gpu->buf[0] >> 24);
            // log_set_quiet(1);

            // exit(1);
        } break;
    }
}

void psx_gpu_write32(psx_gpu_t* gpu, uint32_t offset, uint32_t value) {
    switch (offset) {
        // GP0
        case 0x00: {
            switch (gpu->state) {
                case GPU_STATE_RECV_CMD: {
                    gpu->buf_index = 0;
                    gpu->buf[gpu->buf_index++] = value;

                    psx_gpu_update_cmd(gpu);
                } break;

                case GPU_STATE_RECV_ARGS: {
                    gpu->buf[gpu->buf_index++] = value;
                    gpu->cmd_args_remaining--;

                    psx_gpu_update_cmd(gpu);
                } break;

                case GPU_STATE_RECV_DATA: {
                    gpu->recv_data = value;

                    psx_gpu_update_cmd(gpu);
                } break;
            }

            return;
        } break;

        // GP1
        case 0x04: {
            uint8_t cmd = value >> 24;

            switch (cmd) {
                // Display enable
                case 0x03: {
                    gpu->gpustat &= ~0x00800000;
                    gpu->gpustat |= (value << 23) & 0x00800000;
                } break;
                case 0x04: {
                } break;
                case 0x05: {
                    gpu->disp_x = value & 0x3ff;
                    gpu->disp_y = (value >> 10) & 0x1ff;
                } break;
                case 0x06: {
                    gpu->disp_x1 = value & 0xfff;
                    gpu->disp_x2 = (value >> 12) & 0xfff;
                } break;
                case 0x07: {
                    gpu->disp_y1 = value & 0x1ff;
                    gpu->disp_y2 = (value >> 10) & 0x1ff;
                } break;
                case 0x08:
                    gpu->display_mode = value & 0xffffff;

                    if (gpu->event_cb_table[GPU_EVENT_DMODE])
                        gpu->event_cb_table[GPU_EVENT_DMODE](gpu);
                break;

                case 0x10: {
                    gpu->gp1_10h_req = value & 7;
                } break;
            }

            log_error("GP1(%02Xh) args=%06x", value >> 24, value & 0xffffff);

            return;
        } break;
    }

    log_warn("Unhandled 32-bit GPU write at offset %08x (%08x)", offset, value);
}

void psx_gpu_write16(psx_gpu_t* gpu, uint32_t offset, uint16_t value) {
    printf("Unhandled 16-bit GPU write at offset %08x (%04x)\n", offset, value);
}

void psx_gpu_write8(psx_gpu_t* gpu, uint32_t offset, uint8_t value) {
    printf("Unhandled 8-bit GPU write at offset %08x (%02x)\n", offset, value);
}

void psx_gpu_set_event_callback(psx_gpu_t* gpu, int event, psx_gpu_event_callback_t cb) {
    gpu->event_cb_table[event] = cb;
}

void psx_gpu_set_udata(psx_gpu_t* gpu, int index, void* udata) {
    gpu->udata[index] = udata;
}

#define GPU_CYCLES_PER_HDRAW_NTSC 2560.0f
#define GPU_CYCLES_PER_SCANL_NTSC 3413.0f
#define GPU_SCANS_PER_VDRAW_NTSC 240
#define GPU_SCANS_PER_FRAME_NTSC 263
#define GPU_CYCLES_PER_SCANL_PAL 3406.0f
#define GPU_SCANS_PER_FRAME_PAL  314

void gpu_hblank_event(psx_gpu_t* gpu) {
    if (gpu->line < GPU_SCANS_PER_VDRAW_NTSC) {
        if (gpu->line & 1) {
            gpu->gpustat |= 1 << 31;
        } else {
            gpu->gpustat &= ~(1 << 31);
        }

        // HACK!! More games are fine with this
        // but others, like Dead or Alive, will refuse
        // to boot because this frequency is not fast
        // enough. Sending T2 IRQs every line fixes DoA
        // but breaks a bunch of games, so I'll keep this
        // like this until I actually fix the timers
        // Games that seem to care about T2 timing:
        // - Street Fighter Alpha 2
        // - Dead or Alive
        // - NBA Jam
        // - Doom
        // - Devil Dice
        // - Zanac x Zanac
        // - Soukyugurentai
        // - Mortal Kombat
        // - PaRappa the Rapper
        // - In The Hunt
        // - Crash Bandicoot
        // - Jackie Chan Stuntmaster
        // - etc.
        // Masking with 7 breaks Street Fighter Alpha 2. The game 
        // just stops sending commands to the CDROM while on
        // Player Select. It probably uses T2 IRQs to time
        // GetlocP commands, if the timer is too slow it will
        // break.
        // if (!(gpu->line & 7))
        //     psx_ic_irq(gpu->ic, IC_SPU);
            // psx_ic_irq(gpu->ic, IC_SPU);
    } else {
        gpu->gpustat &= ~(1 << 31);
    }

    gpu->line++;

    if (gpu->line == GPU_SCANS_PER_VDRAW_NTSC) {
        if (gpu->event_cb_table[GPU_EVENT_VBLANK])
            gpu->event_cb_table[GPU_EVENT_VBLANK](gpu);

        psx_ic_irq(gpu->ic, IC_VBLANK);
    } else if (gpu->line == GPU_SCANS_PER_FRAME_NTSC) {
        if (gpu->event_cb_table[GPU_EVENT_VBLANK_END])
            gpu->event_cb_table[GPU_EVENT_VBLANK_END](gpu);

        gpu->line = 0;
    }
}

void psx_gpu_update(psx_gpu_t* gpu, int cyc) {
    int prev_hblank = (gpu->cycles >= GPU_CYCLES_PER_HDRAW_NTSC) &&
                      (gpu->cycles <= GPU_CYCLES_PER_SCANL_NTSC);

    // Convert CPU (~33.8 MHz) cycles to GPU (~53.7 MHz) cycles
    gpu->cycles += (float)cyc * (PSX_GPU_CLOCK_FREQ_NTSC / PSX_CPU_FREQ);

    int curr_hblank = (gpu->cycles >= GPU_CYCLES_PER_HDRAW_NTSC) &&
                      (gpu->cycles <= GPU_CYCLES_PER_SCANL_NTSC);

    if (curr_hblank && !prev_hblank) {
        if (gpu->event_cb_table[GPU_EVENT_HBLANK])
            gpu->event_cb_table[GPU_EVENT_HBLANK](gpu);

        gpu_hblank_event(gpu);
    } else if (prev_hblank && !curr_hblank) {
        if (gpu->event_cb_table[GPU_EVENT_HBLANK_END])
            gpu->event_cb_table[GPU_EVENT_HBLANK_END](gpu);

        gpu->cycles -= (float)GPU_CYCLES_PER_SCANL_NTSC;
    }
}

void* psx_gpu_get_display_buffer(psx_gpu_t* gpu) {
    if (gpu->gpustat & 0x800000)
        return gpu->empty;

    return gpu->vram + (gpu->disp_x + (gpu->disp_y * 1024));
}

void psx_gpu_destroy(psx_gpu_t* gpu) {
    free(gpu->vram);
    free(gpu);
}
