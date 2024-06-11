#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

struct psx_input_t;

typedef struct psx_input_t psx_input_t;

typedef void (*psx_input_write_t)(void*, uint16_t);
typedef uint32_t (*psx_input_read_t)(void*);
typedef void (*psx_input_on_button_press_t)(void*, uint32_t);
typedef void (*psx_input_on_button_release_t)(void*, uint32_t);
typedef void (*psx_input_on_analog_change_t)(void*, uint32_t, uint16_t);
typedef int (*psx_input_query_fifo_t)(void*);

struct psx_input_t {
    void* udata;

    psx_input_write_t write_func;
    psx_input_read_t read_func;
    psx_input_on_button_press_t on_button_press_func;
    psx_input_on_button_release_t on_button_release_func;
    psx_input_on_analog_change_t on_analog_change_func;
    psx_input_query_fifo_t query_fifo_func;
};

psx_input_t* psx_input_create(void);
void psx_input_init(psx_input_t*);
void psx_input_set_write_func(psx_input_t*, psx_input_write_t);
void psx_input_set_read_func(psx_input_t*, psx_input_read_t);
void psx_input_set_on_button_press_func(psx_input_t*, psx_input_on_button_press_t);
void psx_input_set_on_button_release_func(psx_input_t*, psx_input_on_button_release_t);
void psx_input_set_on_analog_change_func(psx_input_t*, psx_input_on_analog_change_t);
void psx_input_set_query_fifo_func(psx_input_t*, psx_input_query_fifo_t);
void psx_input_destroy(psx_input_t*);

#endif