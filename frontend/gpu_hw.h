#ifndef GPU_HW_H
#define GPU_HW_H

#include "psx.h"
#include "SDL2/SDL.h"
#include "screen.h"

void hw_gpu_init(psxe_screen_t* screen);
void gpu_hw_render_triangle(psx_gpu_t* gpu, vertex_t v0, vertex_t v1, vertex_t v2, poly_data_t data, int edge);

#endif