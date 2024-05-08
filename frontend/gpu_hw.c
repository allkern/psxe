#include "gpu_hw.h"
#include "SDL2/SDL.h"

psxe_screen_t* hw_screen = NULL;

SDL_Texture* hw_texture;

void hw_gpu_init(psxe_screen_t* screen) {
    hw_screen = screen;
    hw_texture = SDL_CreateTexture(
        hw_screen->renderer,
        SDL_PIXELFORMAT_BGR555,
        SDL_TEXTUREACCESS_STREAMING,
        256, 256
    );
}

int dither_kernel[] = {
    -4, +0, -3, +1,
    +2, -2, +3, -1,
    -3, +1, -4, +0,
    +3, -1, +2, -2,
};

#define EDGE(a, b, c) ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x))

int _min3(int a, int b, int c) {
    int m = (a <= b) ? a : b;

    return (m <= c) ? m : c;
}

int _max3(int a, int b, int c) {
    int m = (a > b) ? a : b;

    return (m > c) ? m : c;
}

#define BGR555(c) \
    (((c & 0x0000f8) >> 3) | \
     ((c & 0x00f800) >> 6) | \
     ((c & 0xf80000) >> 9))

void gpu_hw_render_triangle(psx_gpu_t* gpu, vertex_t v0, vertex_t v1, vertex_t v2, poly_data_t data, int edge) {
    v0.x *= hw_screen->scale;
    v0.y *= hw_screen->scale;
    v1.x *= hw_screen->scale;
    v1.y *= hw_screen->scale;
    v2.x *= hw_screen->scale;
    v2.y *= hw_screen->scale;

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

    // a.x += gpu->off_x * hw_screen->scale;
    // b.x += gpu->off_x * hw_screen->scale;
    // c.x += gpu->off_x * hw_screen->scale;
    // a.y += gpu->off_y * hw_screen->scale;
    // b.y += gpu->off_y * hw_screen->scale;
    // c.y += gpu->off_y * hw_screen->scale;

    SDL_Vertex v[3];

    v[0].position.x = a.x;
    v[0].position.y = a.y;
    v[1].position.x = b.x;
    v[1].position.y = b.y;
    v[2].position.x = c.x;
    v[2].position.y = c.y;
    v[0].tex_coord.x = a.tx;
    v[0].tex_coord.y = a.ty;
    v[1].tex_coord.x = b.tx;
    v[1].tex_coord.y = b.ty;
    v[2].tex_coord.x = c.tx;
    v[2].tex_coord.y = c.ty;
    v[0].color.a = 0xff;
    v[0].color.r = (a.c >>  0) & 0xff;
    v[0].color.g = (a.c >>  8) & 0xff;
    v[0].color.b = (a.c >> 16) & 0xff;
    v[1].color.a = 0xff;
    v[1].color.r = (b.c >>  0) & 0xff;
    v[1].color.g = (b.c >>  8) & 0xff;
    v[1].color.b = (b.c >> 16) & 0xff;
    v[2].color.a = 0xff;
    v[2].color.r = (c.c >>  0) & 0xff;
    v[2].color.g = (c.c >>  8) & 0xff;
    v[2].color.b = (c.c >> 16) & 0xff;

    int tpx = (data.texp & 0xf) << 6;
    int tpy = (data.texp & 0x10) << 4;

    SDL_UpdateTexture(hw_texture, NULL, &gpu->vram[tpx + (tpy * 1024)], 2048);

    SDL_RenderGeometry(hw_screen->renderer, hw_texture, v, 3, NULL, 0);

    return;

    int xmin = _min3(a.x, b.x, c.x);
    int ymin = _min3(a.y, b.y, c.y);
    int xmax = _max3(a.x, b.x, c.x);
    int ymax = _max3(a.y, b.y, c.y);

    float area = EDGE(a, b, c);

    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            int bc = (x >= (gpu->draw_x1 * 3)) && (x <= (gpu->draw_x2 * 3)) &&
                     (y >= (gpu->draw_y1 * 3)) && (y <= (gpu->draw_y2 * 3));

            if (!bc)
                continue;

            p.x = x;
            p.y = y;

            float z0 = EDGE(b, c, p);
            float z1 = EDGE(c, a, p);
            float z2 = EDGE(a, b, p);

            int e = ((z0 < 0) || (z1 < 0) || (z2 < 0));

            if (transp && edge)
                e = ((z0 < 0) || (z1 <= 0) || (z2 < 0));

            if (e)
                continue;

            uint16_t color = 0;
            uint32_t mod   = 0;

            if (data.attrib & PA_SHADED) {
                int cr = (z0 * ((a.c >>  0) & 0xff) + z1 * ((b.c >>  0) & 0xff) + z2 * ((c.c >>  0) & 0xff)) / area;
                int cg = (z0 * ((a.c >>  8) & 0xff) + z1 * ((b.c >>  8) & 0xff) + z2 * ((c.c >>  8) & 0xff)) / area;
                int cb = (z0 * ((a.c >> 16) & 0xff) + z1 * ((b.c >> 16) & 0xff) + z2 * ((c.c >> 16) & 0xff)) / area;

                int dy = (y - ymin) & 3;
                int dx = (x - xmin) & 3;

                int dither = dither_kernel[dx + (dy * 4)];

                cr += dither;
                cg += dither;
                cb += dither;

                // Saturate (clamp) to 00-ff
                cr = (cr >= 0xff) ? 0xff : ((cr <= 0) ? 0 : cr);
                cg = (cg >= 0xff) ? 0xff : ((cg <= 0) ? 0 : cg);
                cb = (cb >= 0xff) ? 0xff : ((cb <= 0) ? 0 : cb);

                uint32_t rgb = (cb << 16) | (cg << 8) | cr;

                mod = rgb;
            } else {
                mod = data.v[0].c;
            }

            if (data.attrib & PA_TEXTURED) {
                uint32_t tx = ((z0 * a.tx) + (z1 * b.tx) + (z2 * c.tx)) / area;
                uint32_t ty = ((z0 * a.ty) + (z1 * b.ty) + (z2 * c.ty)) / area;

                uint16_t texel = gpu_fetch_texel(gpu, tx, ty, tpx, tpy, clutx, cluty, depth);

                if (!texel)
                    continue;

                if (transp)
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

                    unsigned int ucr = cr;
                    unsigned int ucg = cg;
                    unsigned int ucb = cb;

                    uint32_t rgb = ucr | (ucg << 8) | (ucb << 16);

                    color = BGR555(rgb);
                }
            } else {
                color = BGR555(mod);
            }

            float cr = ((color >> 0 ) & 0x1f) << 3;
            float cg = ((color >> 5 ) & 0x1f) << 3;
            float cb = ((color >> 10) & 0x1f) << 3;

            // if (transp) {
            //     uint16_t back = gpu->vram[x + (y * 1024)];

            //     float br = ((back >> 0 ) & 0x1f) << 3;
            //     float bg = ((back >> 5 ) & 0x1f) << 3;
            //     float bb = ((back >> 10) & 0x1f) << 3;

            //     // Do we use transp or gpustat here?
            //     switch (transp_mode) {
            //         case 0: {
            //             cr = (0.5f * br) + (0.5f * cr);
            //             cg = (0.5f * bg) + (0.5f * cg);
            //             cb = (0.5f * bb) + (0.5f * cb);
            //         } break;
            //         case 1: {
            //             cr = br + cr;
            //             cg = bg + cg;
            //             cb = bb + cb;
            //         } break;
            //         case 2: {
            //             cr = br - cr;
            //             cg = bg - cg;
            //             cb = bb - cb;
            //         } break;
            //         case 3: {
            //             cr = br + (0.25 * cr);
            //             cg = bg + (0.25 * cg);
            //             cb = bb + (0.25 * cb);
            //         } break;
            //     }

            //     cr = (cr >= 255.0f) ? 255.0f : ((cr <= 0.0f) ? 0.0f : cr);
            //     cg = (cg >= 255.0f) ? 255.0f : ((cg <= 0.0f) ? 0.0f : cg);
            //     cb = (cb >= 255.0f) ? 255.0f : ((cb <= 0.0f) ? 0.0f : cb);

            //     unsigned int ucr = cr;
            //     unsigned int ucg = cg;
            //     unsigned int ucb = cb;

            //     uint32_t rgb = ucr | (ucg << 8) | (ucb << 16);

            //     color = BGR555(rgb);
            // }

            SDL_Vertex v[3];

            v[0].position.x = a.x;
            v[0].position.y = a.y;
            v[1].position.x = b.x;
            v[1].position.y = b.y;
            v[2].position.x = c.x;
            v[2].position.y = c.y;
            v[0].color.a = 0xff;
            v[0].color.r = (a.c >>  0) & 0xff;
            v[0].color.g = (a.c >>  8) & 0xff;
            v[0].color.b = (a.c >> 16) & 0xff;
            v[1].color.a = 0xff;
            v[1].color.r = (b.c >>  0) & 0xff;
            v[1].color.g = (b.c >>  8) & 0xff;
            v[1].color.b = (b.c >> 16) & 0xff;
            v[2].color.a = 0xff;
            v[2].color.r = (c.c >>  0) & 0xff;
            v[2].color.g = (c.c >>  8) & 0xff;
            v[2].color.b = (c.c >> 16) & 0xff;

            SDL_RenderGeometry(hw_screen->renderer, NULL, v, 3, NULL, 0);

            // gpu->vram[x + (y * 1024)] = color;
        }
    }
}