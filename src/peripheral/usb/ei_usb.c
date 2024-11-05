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

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "common_events.h"

#include <stdint.h>
#include "ei_usb.h"
#include "stdbool.h"
#include <string.h>

#include "ns_core.h"
#include "ns_usb.h"

static EventGroupHandle_t usb_event_group;

// usb task parameters
#define USB_TASK_STACK_SIZE_BYTE        (4096u)
#define USB_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
static TaskHandle_t usb_task_handle;
static void usb_task(void *pvParameters);

#define USB_EVENT_RX_READY  (1 << 0)
#define USB_EVENT_TX_DONE   (1 << 1)

#define MY_RX_BUFSIZE 4096
#define MY_TX_BUFSIZE 4096

AM_SHARED_RW static uint8_t my_rx_ff_buf[MY_RX_BUFSIZE] = {0xFF};
AM_SHARED_RW static uint8_t my_tx_ff_buf[MY_TX_BUFSIZE];

/* Counter to update g_temp_buffer index */
static uint32_t g_rx_index = 0;
/* Index of data sent to at hanlder */
static uint32_t g_uart_rx_read_index = 0;

static bool _usb_is_init = false;
static bool _is_clear = false;
static usb_handle_t usb_handle;

// callbacks
static void ei_usb_rx_cb(ns_usb_transaction_t *pTransaction);
static void ei_usb_tx_cb(ns_usb_transaction_t *pTransaction);
static void ei_usb_service_cb(uint8_t service);

static void usb_local_read(void);

static ns_usb_config_t usb_cdc_config = {
    .api = &ns_usb_V1_0_0,
    .deviceType = NS_USB_CDC_DEVICE,
    .rx_buffer = my_rx_ff_buf,
    .rx_bufferLength = MY_RX_BUFSIZE,
    .tx_buffer = my_tx_ff_buf,
    .tx_bufferLength = MY_TX_BUFSIZE,
    .rx_cb = ei_usb_rx_cb,
    .tx_cb = ei_usb_tx_cb,
    .service_cb = ei_usb_service_cb};

/**
 * @brief 
 * 
 * @return int 
 */
int ei_usb_init(void)
{
    BaseType_t retval;

    usb_event_group = xEventGroupCreate();
    NS_TRY(ns_usb_init(&usb_cdc_config, &usb_handle), "USB Init Failed\n");

    /* create a task to send data via usb */
    retval = xTaskCreate(usb_task,
        (const char*) "USB Thread",
        USB_TASK_STACK_SIZE_BYTE / 4, // in words
        NULL, //pvParameters
        USB_TASK_PRIORITY, //uxPriority
        &usb_task_handle);

    return (int)retval;
}

/**
 * @brief 
 * 
 * @param pvParameters 
 */
static void usb_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        usb_local_read();
        vTaskDelay(1);
    }
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool usb_is_init(void)
{
    return _usb_is_init;
}

/**
 * @brief 
 * 
 * @param buffer 
 * @param length 
 */
void ei_usb_send(uint8_t *buffer, uint32_t length)
{
    ns_usb_send_data(usb_handle, buffer, length);

#if 0
    xEventGroupWaitBits(usb_event_group, 
                                    USB_EVENT_TX_DONE,    //  uxBitsToWaitFor 
                                    pdTRUE,                 //  xClearOnExit
                                    pdFALSE,                //  xWaitForAllBits
                                    portMAX_DELAY);
#endif
}

/**
 * @brief Handles blockin read
 */
static void usb_local_read(void)
{
    uint32_t read = 0;
    EventBits_t event_bit;

    event_bit = xEventGroupWaitBits(usb_event_group, 
                                    USB_EVENT_RX_READY,    //  uxBitsToWaitFor 
                                    pdTRUE,                 //  xClearOnExit
                                    pdFALSE,                //  xWaitForAllBits
                                    portMAX_DELAY);

    if (event_bit & USB_EVENT_RX_READY) {
        while(my_rx_ff_buf[g_rx_index] != 0x00){ 
            g_rx_index++;
        }
        _is_clear = false;


        xEventGroupSetBits(
                        common_event_group,      /* The event group being updated. */
                        EVENT_RX_READY);      /* The bits being set. */
    }
}

/**
 * @brief 
 * 
 * @param pTransaction 
 */
static void ei_usb_rx_cb(ns_usb_transaction_t * pTransaction)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventBits_t uxBits;
    
    uxBits = xEventGroupSetBitsFromISR(
                              usb_event_group,      /* The event group being updated. */
                              USB_EVENT_RX_READY,     /* The bits being set. */
                              &xHigherPriorityTaskWoken);
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief 
 * 
 * @param pTransaction 
 */
static void ei_usb_tx_cb(ns_usb_transaction_t *pTransaction)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventBits_t uxBits;
    
    uxBits = xEventGroupSetBitsFromISR(
                              usb_event_group,      /* The event group being updated. */
                              USB_EVENT_TX_DONE,      /* The bits being set. */
                              &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static volatile uint8_t local_service;

/**
 * @brief 
 * 
 * @param service 
 */
static void ei_usb_service_cb(uint8_t service)
{
    local_service = service;
}

/**
 * @brief Returns char from uart rx buffer
 *
 * @param is_inference_running If inference is running, we need to check for a single 'b'
 * @return
 */
char ei_get_serial_byte(uint8_t is_inference_running)
{
    char to_send = -1;

    if (g_uart_rx_read_index < g_rx_index) {
        to_send = my_rx_ff_buf[g_uart_rx_read_index];    // get one

        g_uart_rx_read_index = (g_uart_rx_read_index + 1)%MY_RX_BUFSIZE;  // increment and wrap
    }
    else if ((g_uart_rx_read_index == g_rx_index) && (_is_clear == false)) {  // when equal and different from zero
        memset(my_rx_ff_buf, 0x00, sizeof(my_rx_ff_buf));   // no reset, just clear everything
        _is_clear = true;
    }

    return to_send;
}
