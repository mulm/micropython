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

#ifndef BLUETOOTH_LE_BAS_H__
#define BLUETOOTH_LE_BAS_H__

#if BLUETOOTH_SD

#include "modubluepy.h"
#include "ble_drv.h"

void ble_bas_init0(ubluepy_peripheral_obj_t * p_ble_peripheral, mp_obj_t service_list);
void gap_event_handler_bas(mp_obj_t self_in, uint16_t event_id, uint16_t conn_handle, uint16_t length, uint8_t * data);
void gatts_event_handler_bas(mp_obj_t self_in, uint16_t event_id, uint16_t attr_handle, uint16_t length, uint8_t * data);

#endif // BLUETOOTH_SD

#endif // BLUETOOTH_LE_BAS_H__
