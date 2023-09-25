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
    sda->tx_data_ready = 1;
    sda->model = model;
    sda->state = SDA_STATE_TX_HIZ;
    sda->sw = 0xffff;
}

uint32_t psxi_sda_read(void* udata) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    switch (sda->state) {
        case SDA_STATE_TX_HIZ: sda->tx_data = 0xff; break;
        case SDA_STATE_TX_IDL: sda->tx_data = sda->model; break;
        case SDA_STATE_TX_IDH: sda->tx_data = 0x5a; break;
        case SDA_STATE_TX_SWL: sda->tx_data = sda->sw & 0xff; break;

        // Last state
        case SDA_STATE_TX_SWH: {
            sda->tx_data_ready = 0;
            sda->state = 0;

            return sda->sw >> 8;
        } break;
    }

    sda->tx_data_ready = 1;
    sda->state++;

    return sda->tx_data;
}

void psxi_sda_write(void* udata, uint16_t data) {
    // To-do: Handle TAP and MOT bytes here

    return;
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
    // Suppress warning until we implement analog mode
    // psxi_sda_t* sda = (psxi_sda_t*)udata;
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