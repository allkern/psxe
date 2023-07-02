#ifndef PAD_H
#define PAD_H

#include <stdint.h>

#include "ic.h"

#define PSX_PAD_BEGIN 0x1f801040
#define PSX_PAD_SIZE  0x10
#define PSX_PAD_END   0x1f80104f

// Controller/Input IDs
#define PSXI_ID_SD          0x5a41
#define PSXI_ID_SA_PAD      0x5a73
#define PSXI_ID_SA_STICK    0x5a53

#define PSXI_SW_SDA_SELECT      0x0001
#define PSXI_SW_SDA_L3          0x0002
#define PSXI_SW_SDA_R3          0x0004
#define PSXI_SW_SDA_START       0x0008
#define PSXI_SW_SDA_PAD_UP      0x0010
#define PSXI_SW_SDA_PAD_RIGHT   0x0020
#define PSXI_SW_SDA_PAD_DOWN    0x0040
#define PSXI_SW_SDA_PAD_LEFT    0x0080
#define PSXI_SW_SDA_L2          0x0100
#define PSXI_SW_SDA_R2          0x0200
#define PSXI_SW_SDA_L1          0x0400
#define PSXI_SW_SDA_R1          0x0800
#define PSXI_SW_SDA_TRIANGLE    0x1000
#define PSXI_SW_SDA_CIRCLE      0x2000
#define PSXI_SW_SDA_CROSS       0x4000
#define PSXI_SW_SDA_SQUARE      0x8000

/*
    To-do: Design API to interface any type of controller.

    Possible names:
    - psx_im (Input Method)
    - psx_controller
    - psx_input

    Private API should contain a way to get the ID of
    this controller, public API should contain the following
    functions: (WIP)
    - _write(data)
    - _read()
    _ _on_button_press(id)
    - _on_button_release(id)
    - _on_analog_change(id)
*/

typedef struct {
    uint32_t io_base, io_size;

    psx_ic_t* ic;

    uint8_t rx_buf[6];
    int rx_index;

    uint16_t mode, ctrl, baud;
} psx_pad_t;

psx_pad_t* psx_pad_create();
void psx_pad_init(psx_pad_t*, psx_ic_t*);
uint32_t psx_pad_read32(psx_pad_t*, uint32_t);
uint16_t psx_pad_read16(psx_pad_t*, uint32_t);
uint8_t psx_pad_read8(psx_pad_t*, uint32_t);
void psx_pad_write32(psx_pad_t*, uint32_t, uint32_t);
void psx_pad_write16(psx_pad_t*, uint32_t, uint16_t);
void psx_pad_write8(psx_pad_t*, uint32_t, uint8_t);
void psx_pad_destroy(psx_pad_t*);
void psx_pad_button_press(psx_pad_t*, uint16_t);
void psx_pad_button_release(psx_pad_t*, uint16_t);

#endif