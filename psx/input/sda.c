/*
    This file is part of the PSXE Emulator Project

    Sony PlayStation Standard Digital/Analog Controller Emulator
*/

#include "sda.h"
#include "../log.h"

#include <stdlib.h>
#include <string.h>

psxi_sda_t* psxi_sda_create(void) {
    return (psxi_sda_t*)malloc(sizeof(psxi_sda_t));
}

void psxi_sda_init(psxi_sda_t* sda, uint16_t model) {
    memset(sda, 0, sizeof(psxi_sda_t));

    sda->tx_data = 0xff;
    sda->tx_data_ready = 1;
    sda->prev_model = model;
    sda->model = model;
    sda->state = SDA_STATE_TX_HIZ;
    sda->sw = 0xffff;
    sda->sa_mode = SA_MODE_DIGITAL;
    sda->adc0 = 0x80;
    sda->adc1 = 0x80;
    sda->adc2 = 0x80;
    sda->adc3 = 0x80;
}

uint32_t psxi_sda_read(void* udata) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    switch (sda->state) {
        case SDA_STATE_TX_HIZ: sda->tx_data = 0xff; break;
        case SDA_STATE_TX_IDL: sda->tx_data = sda->model; break;
        case SDA_STATE_TX_IDH: sda->tx_data = 0x5a; break;
        case SDA_STATE_TX_SWL: sda->tx_data = sda->sw & 0xff; break;

        // Digital pad stops sending data here
        case SDA_STATE_TX_SWH: {
            if (sda->sa_mode == SA_MODE_ANALOG) {
                sda->tx_data_ready = 1;
                sda->state = SDA_STATE_TX_ADC0;
            } else {
                sda->tx_data_ready = 0;
                sda->state = SDA_STATE_TX_HIZ;
            }

            return sda->sw >> 8;
        } break;

        case SDA_STATE_TX_ADC0: sda->tx_data = sda->adc0; break;
        case SDA_STATE_TX_ADC1: sda->tx_data = sda->adc1; break;
        case SDA_STATE_TX_ADC2: sda->tx_data = sda->adc2; break;

        // Analog pad stops sending data here
        case SDA_STATE_TX_ADC3: {
            sda->tx_data_ready = 0;
            sda->state = SDA_STATE_TX_HIZ;

            if (sda->model == 0xf3)
                sda->model = sda->prev_model;

            return sda->adc3;
        } break;
        
    }

    // printf("  sda read %u -> %02x\n", sda->state, sda->tx_data);

    sda->tx_data_ready = 1;
    sda->state++;

    return sda->tx_data;
}

void psxi_sda_write(void* udata, uint16_t data) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    // To-do: Handle TAP and MOT bytes here

    if (data == 0x43) {
        if (sda->sa_mode == SA_MODE_ANALOG) {
            sda->prev_model = sda->model;
            sda->model = 0xf3;
        } else {
            sda->tx_data = 0xff;
            sda->tx_data_ready = 0;
            sda->state = SDA_STATE_TX_HIZ;
        }
    }
}

void psxi_sda_on_button_press(void* udata, uint32_t data) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    if (data == PSXI_SW_SDA_ANALOG) {
        sda->sa_mode ^= 1;

        if (sda->sa_mode) {
            sda->prev_model = sda->model;
            sda->model = SDA_MODEL_ANALOG_STICK;
        } else {
            sda->model = sda->prev_model;
        }

        printf("sda: Switched to %s mode\n", sda->sa_mode ? "analog" : "digital");

        return;
    }

    sda->sw &= ~data;
}

void psxi_sda_on_button_release(void* udata, uint32_t data) {
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    sda->sw |= data;
}

// To-do: Implement analog mode
void psxi_sda_on_analog_change(void* udata, uint32_t axis, uint16_t data) {
    // Suppress warning until we implement analog mode
    psxi_sda_t* sda = (psxi_sda_t*)udata;

    switch (axis) {
        case PSXI_AX_SDA_RIGHT_HORZ: sda->adc0 = data; break;
        case PSXI_AX_SDA_RIGHT_VERT: sda->adc1 = data; break;
        case PSXI_AX_SDA_LEFT_HORZ: sda->adc2 = data; break;
        case PSXI_AX_SDA_LEFT_VERT: sda->adc3 = data; break;
    }
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