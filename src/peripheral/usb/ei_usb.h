/*
 * Copyright (c) 2024 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _EI_USB_H_
#define _EI_USB_H_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern int ei_usb_init(void);
extern void ei_usb_send(uint8_t *buffer, uint32_t length);
extern char ei_get_serial_byte(uint8_t is_inference_running);

#if defined(__cplusplus)
}
#endif

#endif
