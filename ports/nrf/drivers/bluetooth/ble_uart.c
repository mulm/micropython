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

#if BLUETOOTH_SD

#include <string.h>
#include "ble_uart.h"
#include "ringbuffer.h"
#include "mphalport.h"
#include "lib/utils/interrupt_char.h"
#include "wdt.h"

#if MICROPY_PY_BLE_NUS

STATIC bool ble_uart_enabled(void);

static ubluepy_uuid_obj_t uuid_obj_service = {
    .base.type = &ubluepy_uuid_type,
    .type = UBLUEPY_UUID_128_BIT,
    .value = {0x01, 0x00}
};

static ubluepy_uuid_obj_t uuid_obj_char_tx = {
    .base.type = &ubluepy_uuid_type,
    .type = UBLUEPY_UUID_128_BIT,
    .value = {0x03, 0x00}
};

static ubluepy_uuid_obj_t uuid_obj_char_rx = {
    .base.type = &ubluepy_uuid_type,
    .type = UBLUEPY_UUID_128_BIT,
    .value = {0x02, 0x00}
};

static ubluepy_service_obj_t ble_uart_service = {
    .base.type = &ubluepy_service_type,
    .p_uuid = &uuid_obj_service,
    .type = UBLUEPY_SERVICE_PRIMARY
};

static ubluepy_characteristic_obj_t ble_uart_char_rx = {
    .base.type = &ubluepy_characteristic_type,
    .p_uuid = &uuid_obj_char_rx,
    .props = UBLUEPY_PROP_WRITE | UBLUEPY_PROP_WRITE_WO_RESP,
    .attrs = 0,
};

static ubluepy_characteristic_obj_t ble_uart_char_tx = {
    .base.type = &ubluepy_characteristic_type,
    .p_uuid = &uuid_obj_char_tx,
    .props = UBLUEPY_PROP_NOTIFY,
    .attrs = UBLUEPY_ATTR_CCCD,
};

static volatile bool m_cccd_enabled;

ringBuffer_typedef(uint8_t, ringbuffer_t);

static ringbuffer_t   m_rx_ring_buffer;
static ringbuffer_t * mp_rx_ring_buffer = &m_rx_ring_buffer;
static uint8_t        m_rx_ring_buffer_data[128];

int mp_hal_stdin_rx_chr(void) {
    wdt_feed(false);

    while (!ble_uart_enabled()) {
        // wait for connection
	wdt_feed(false);
    }
    while (isBufferEmpty(mp_rx_ring_buffer)) {
	wdt_feed(false);
    }

    uint8_t byte;
    bufferRead(mp_rx_ring_buffer, byte);
    return (int)byte;
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    // Not connected: drop output
    if (!ble_uart_enabled()) return;

    uint8_t *buf = (uint8_t *)str;
    size_t send_len;

    while (len > 0) {
        if (len >= 20) {
            send_len = 20; // (GATT_MTU_SIZE_DEFAULT - 3)
        } else {
            send_len = len;
        }

        ubluepy_characteristic_obj_t * p_char = &ble_uart_char_tx;

        ble_drv_attr_s_notify(p_char->p_service->p_periph->conn_handle,
                              p_char->handle,
                              send_len,
                              buf);

        len -= send_len;
        buf += send_len;
    }
}

void mp_hal_stdout_tx_strn_cooked(const char *str, mp_uint_t len) {
    mp_hal_stdout_tx_strn(str, len);
}

void gap_event_handler_uart(mp_obj_t self_in, uint16_t event_id, uint16_t conn_handle, uint16_t length, uint8_t * data) {

    if (event_id == 17) {         // disconnect event
        m_cccd_enabled = false;
    }
}

void gatts_event_handler_uart(mp_obj_t self_in, uint16_t event_id, uint16_t attr_handle, uint16_t length, uint8_t * data) {
    ubluepy_peripheral_obj_t * self = MP_OBJ_TO_PTR(self_in);
    (void)self;

    if (event_id == 80) { // gatts write
        if (ble_uart_char_tx.cccd_handle == attr_handle) {
            m_cccd_enabled = true;
        } else if (ble_uart_char_rx.handle == attr_handle) {
            for (uint16_t i = 0; i < length; i++) {
                #if MICROPY_KBD_EXCEPTION
                if (data[i] == mp_interrupt_char) {
                    mp_keyboard_interrupt();
                    m_rx_ring_buffer.start = 0;
                    m_rx_ring_buffer.end = 0;
                } else
                #endif
                {
                    bufferWrite(mp_rx_ring_buffer, data[i]);
                }
            }
        }
    }
}

void ble_uart_init0(ubluepy_peripheral_obj_t * p_ble_peripheral, mp_obj_t service_list) {
    uint8_t base_uuid[] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x00, 0x00, 0x40, 0x6E};
    uint8_t uuid_vs_idx;

    (void)ble_drv_uuid_add_vs(base_uuid, &uuid_vs_idx);

    uuid_obj_service.uuid_vs_idx = uuid_vs_idx;
    uuid_obj_char_tx.uuid_vs_idx = uuid_vs_idx;
    uuid_obj_char_rx.uuid_vs_idx = uuid_vs_idx;

    (void)ble_drv_service_add(&ble_uart_service);
    ble_uart_service.char_list = mp_obj_new_list(0, NULL);

    // add TX characteristic
    ble_uart_char_tx.service_handle = ble_uart_service.handle;
    bool retval = ble_drv_characteristic_add(&ble_uart_char_tx);
    if (retval) {
        ble_uart_char_tx.p_service = &ble_uart_service;
    }
    mp_obj_list_append(ble_uart_service.char_list, MP_OBJ_FROM_PTR(&ble_uart_char_tx));

    // add RX characteristic
    ble_uart_char_rx.service_handle = ble_uart_service.handle;
    retval = ble_drv_characteristic_add(&ble_uart_char_rx);
    if (retval) {
        ble_uart_char_rx.p_service = &ble_uart_service;
    }
    mp_obj_list_append(ble_uart_service.char_list, MP_OBJ_FROM_PTR(&ble_uart_char_rx));

    // setup the peripheral
    mp_obj_list_append(p_ble_peripheral->service_list, MP_OBJ_FROM_PTR(&ble_uart_service));
    ble_uart_service.p_periph = p_ble_peripheral;

    mp_obj_list_append(service_list, MP_OBJ_FROM_PTR(&ble_uart_service));

    m_cccd_enabled = false;

    // initialize ring buffer
    m_rx_ring_buffer.size = sizeof(m_rx_ring_buffer_data) + 1;
    m_rx_ring_buffer.start = 0;
    m_rx_ring_buffer.end = 0;
    m_rx_ring_buffer.elems = m_rx_ring_buffer_data;
}

bool ble_uart_enabled(void) {
    return (m_cccd_enabled);
}

#endif // MICROPY_PY_BLE_NUS

#endif // BLUETOOTH_SD
