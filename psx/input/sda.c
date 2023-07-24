/*
    This file is part of the PSXE Emulator Project

    Sony PlayStation Standard Digital/Analog Controller Emulator
*/

#include "sda.h"
#include "../log.h"

#include <stdlib.h>
#include <string.h>

psxi_sda_t* psxi_sda_create() {
    return (psxi_sda_t*)malloc(sizeof(psxi_sda_t));
}

void psxi_sda_init(psxi_sda_t* sda, uint16_t model) {
    memset(sda, 0, sizeof(psxi_sda_t));

    sda->tx_data = 0xff;
    sda->model = model;
    sda->state = SDA_STATE_WFC;
    sda->sw = 0xffff;
}

uint32_t psxi_sda_read(void* udata) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    return sda->tx_data;
}

void psxi_sda_write(void* udata, uint16_t data) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    switch (sda->state) {
        case SDA_STATE_WFC: {
            if (data == 0x01) {
                sda->tx_data_ready = 1;
                sda->tx_data = 0xff;
                sda->state = SDA_STATE_WFR;
            }

            // Memory card access
            if (data == 0x81) {
                sda->tx_data_ready = 1;
                sda->tx_data = 0xff;
                sda->state = SDA_STATE_WFR;
            }
        } break;

        case SDA_STATE_WFR: {
            if (data == 'B') {
                sda->tx_data = sda->model;
                sda->state = SDA_STATE_TX_IDH;
            } else if (data == 'R') {
                sda->tx_data = 0xff;
                sda->state = SDA_STATE_WFR;
            }
        } break;

        case SDA_STATE_TX_IDH: {
            // To-do: Handle MOT
            sda->tx_data = 0x5a;
            sda->state = SDA_STATE_TX_SWL;
        } break;

        case SDA_STATE_TX_SWL: {
            sda->tx_data = sda->sw & 0xff;
            sda->state = SDA_STATE_TX_SWH;
        } break;

        case SDA_STATE_TX_SWH: {
            sda->tx_data = sda->sw >> 8;

            switch (sda->model) {
                case 0x41:
                    sda->state = SDA_STATE_WFC;
                break;

                // To-do: Implement analog mode
                case 0x73:
                case 0x53:
                    sda->state = SDA_STATE_WFC;
                break;
            }
        } break;
    }
}

void psxi_sda_on_button_press(void* udata, uint16_t data) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    sda->sw &= ~data;
}

void psxi_sda_on_button_release(void* udata, uint16_t data) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    sda->sw |= data;
}

// To-do: Implement analog mode
void psxi_sda_on_analog_change(void* udata, uint16_t data) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;
}

int psxi_sda_query_fifo(void* udata) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    return sda->tx_data_ready;
}

void psxi_sda_init_input(psxi_sda_t* sda, psx_input_t* input) {
    input->udata = sda;
    input->write_func = psxi_sda_write;
    input->read_func = psxi_sda_read;
    input->on_button_press_func = psxi_sda_on_button_press;
    input->on_button_release_func = psxi_sda_on_button_release;
    input->on_analog_change_func = psxi_sda_on_analog_change;
    input->query_fifo_func = psxi_sda_query_fifo;
}

void psxi_sda_destroy(psxi_sda_t* sda) {
    free(sda);
}