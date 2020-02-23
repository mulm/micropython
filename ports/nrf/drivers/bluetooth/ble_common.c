/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Glenn Ruben Bakke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>
#include "ble_common.h"
#include "ble_uart.h"

#if BLUETOOTH_SD

STATIC void ble_uart_advertise(void);

static ubluepy_peripheral_obj_t ble_uart_peripheral = {
    .base.type = &ubluepy_peripheral_type,
    .conn_handle = 0xFFFF,
};

static volatile bool m_connected;

static ubluepy_advertise_data_t m_adv_data_uart_service;

#if BLUETOOTH_WEBBLUETOOTH_REPL
static ubluepy_advertise_data_t m_adv_data_eddystone_url;
#endif // BLUETOOTH_WEBBLUETOOTH_REPL

STATIC void gap_event_handler(mp_obj_t self_in, uint16_t event_id, uint16_t conn_handle, uint16_t length, uint8_t * data) {
    ubluepy_peripheral_obj_t * self = MP_OBJ_TO_PTR(self_in);

    if (event_id == 16) {                // connect event
        self->conn_handle = conn_handle;
        m_connected = true;
    } else if (event_id == 17) {         // disconnect event
        self->conn_handle = 0xFFFF;      // invalid connection handle
        m_connected = false;
        ble_uart_advertise();
    }
    gap_event_handler_uart(self_in, event_id, conn_handle, length, data);
}

STATIC void gatts_event_handler(mp_obj_t self_in, uint16_t event_id, uint16_t attr_handle, uint16_t length, uint8_t * data) {
    gatts_event_handler_uart(self_in, event_id, attr_handle, length, data);
}

void ble_init0(void)
{
    ble_uart_peripheral.service_list = mp_obj_new_list(0, NULL);
    mp_obj_t service_list = mp_obj_new_list(0, NULL);

#if MICROPY_PY_BLE_NUS
    ble_uart_init0(&ble_uart_peripheral, service_list);
#endif

    ble_drv_gap_event_handler_set(MP_OBJ_FROM_PTR(&ble_uart_peripheral), gap_event_handler);
    ble_drv_gatts_event_handler_set(MP_OBJ_FROM_PTR(&ble_uart_peripheral), gatts_event_handler);

    ble_uart_peripheral.conn_handle = 0xFFFF;

#ifdef MICROPY_HW_NUS_NAME
    char device_name[] = MICROPY_HW_NUS_NAME;
#else
    char device_name[] = "mpus";
#endif

    mp_obj_t * services = NULL;
    mp_uint_t  num_services;
    mp_obj_get_array(service_list, &num_services, &services);

    m_adv_data_uart_service.p_services      = services;
    m_adv_data_uart_service.num_of_services = num_services;
    m_adv_data_uart_service.p_device_name   = (uint8_t *)device_name;
    m_adv_data_uart_service.device_name_len = strlen(device_name);
    m_adv_data_uart_service.connectable     = true;
    m_adv_data_uart_service.p_data          = NULL;

#if BLUETOOTH_WEBBLUETOOTH_REPL
    // for now point eddystone URL to https://goo.gl/F7fZ69 => https://aykevl.nl/apps/nus/
    static uint8_t eddystone_url_data[27] = {0x2, 0x1, 0x6,
                                             0x3, 0x3, 0xaa, 0xfe,
                                             19, 0x16, 0xaa, 0xfe, 0x10, 0xee, 0x3, 'g', 'o', 'o', '.', 'g', 'l', '/', 'F', '7', 'f', 'Z', '6', '9'};
    // eddystone url adv data
    m_adv_data_eddystone_url.p_data      = eddystone_url_data;
    m_adv_data_eddystone_url.data_len    = sizeof(eddystone_url_data);
    m_adv_data_eddystone_url.connectable = false;
#endif

    m_connected = false;

    ble_uart_advertise();
}

void ble_uart_advertise(void) {
#if BLUETOOTH_WEBBLUETOOTH_REPL
    while (!m_connected) {
        (void)ble_drv_advertise_data(&m_adv_data_uart_service);
        mp_hal_delay_ms(500);
        (void)ble_drv_advertise_data(&m_adv_data_eddystone_url);
        mp_hal_delay_ms(500);
    }

    ble_drv_advertise_stop();
#else
    (void)ble_drv_advertise_data(&m_adv_data_uart_service);
#endif // BLUETOOTH_WEBBLUETOOTH_REPL
}

#endif // BLUETOOTH_SD
