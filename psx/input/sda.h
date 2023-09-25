/*
    This file is part of the PSXE Emulator Project

    Sony PlayStation Standard Digital/Analog Controller Emulator
*/

#ifndef SDA_H
#define SDA_H

#include "../dev/input.h"

// Controller/Input IDs
#define SDA_MODEL_DIGITAL       0x41
#define SDA_MODEL_ANALOG_PAD    0x73
#define SDA_MODEL_ANALOG_STICK  0x53

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

enum {
    SDA_STATE_TX_HIZ = 0,
    SDA_STATE_TX_IDL,
    SDA_STATE_TX_IDH,
    SDA_STATE_TX_SWL,
    SDA_STATE_TX_SWH
};

enum {
    SA_MODE_DIGITAL = 0,
    SA_MODE_ANALOG
};

typedef struct {
    uint8_t model;
    int state;
    int sa_mode;
    uint16_t sw;
    uint8_t tx_data;
    int tx_data_ready;
} psxi_sda_t;

psxi_sda_t* psxi_sda_create();
void psxi_sda_init(psxi_sda_t*, uint16_t);
void psxi_sda_init_input(psxi_sda_t*, psx_input_t*);
void psxi_sda_destroy(psxi_sda_t*);

#endif