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
#include "ble_types.h"
#include "ble_bas.h"

#if MICROPY_PY_BLE_BAS

static ubluepy_uuid_obj_t uuid_obj_bat_service = {
    .base.type = &ubluepy_uuid_type,
    .type = BLE_UUID_TYPE_BLE,
    .value = {0x0F, 0x18}
};

static ubluepy_uuid_obj_t uuid_obj_char_bat_level = {
    .base.type = &ubluepy_uuid_type,
    .type = BLE_UUID_TYPE_BLE,
    .value = {0x19, 0x2A}
};

static ubluepy_service_obj_t ble_bas_service = {
    .base.type = &ubluepy_service_type,
    .p_uuid = &uuid_obj_bat_service,
    .type = UBLUEPY_SERVICE_PRIMARY
};

static ubluepy_characteristic_obj_t ble_bas_char_bat_level = {
    .base.type = &ubluepy_characteristic_type,
    .p_uuid = &uuid_obj_char_bat_level,
    .props = UBLUEPY_PROP_READ,
    .attrs = 0,
};

void gap_event_handler_bas(mp_obj_t self_in, uint16_t event_id, uint16_t conn_handle, uint16_t length, uint8_t * data) {
}

void gatts_event_handler_bas(mp_obj_t self_in, uint16_t event_id, uint16_t attr_handle, uint16_t length, uint8_t * data) {
    ubluepy_peripheral_obj_t * self = MP_OBJ_TO_PTR(self_in);
    (void)self;

    if (event_id == 80) { // gatts write
    }
}

void ble_bas_init0(ubluepy_peripheral_obj_t * p_ble_peripheral, mp_obj_t service_list) {
    (void)ble_drv_service_add(&ble_bas_service);
    ble_bas_service.char_list = mp_obj_new_list(0, NULL);

    // add RX characteristic
    ble_bas_char_bat_level.service_handle = ble_bas_service.handle;
    if (ble_drv_characteristic_add(&ble_bas_char_bat_level)) {
        ble_bas_char_bat_level.p_service = &ble_bas_service;
    }
    mp_obj_list_append(ble_bas_service.char_list, MP_OBJ_FROM_PTR(&ble_bas_char_bat_level));

    // setup the peripheral
    mp_obj_list_append(p_ble_peripheral->service_list, MP_OBJ_FROM_PTR(&ble_bas_service));
    ble_bas_service.p_periph = p_ble_peripheral;

    mp_obj_list_append(service_list, MP_OBJ_FROM_PTR(&ble_bas_service));
}

#endif // MICROPY_PY_BLE_BAS

#endif // BLUETOOTH_SD
