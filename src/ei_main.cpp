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

#include "peripheral/peripheral.h"
#include "peripheral/usb/ei_usb.h"
#include "common_events.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"

#if (configAPPLICATION_ALLOCATED_HEAP == 1)
size_t ucHeapSize = (NS_MALLOC_HEAP_SIZE_IN_K + 4) * 1024;
uint8_t ucHeap[(NS_MALLOC_HEAP_SIZE_IN_K + 4) * 1024] __attribute__((aligned(4)));
#endif

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_variables.h"

// main task parameters
#define EI_MAIN_TASK_STACK_SIZE_BYTE        (4096u)
#define EI_MAIN_TASK_PRIORITY               (configMAX_PRIORITIES - 2)
static TaskHandle_t ei_main_task_handle;
static void ei_main_task(void *pvParameters);

EventGroupHandle_t common_event_group;

static const float features[] = {
    // copy raw features here
};

static int raw_feature_get_data(size_t offset, size_t length, float *out_ptr)
{
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

/**
 * @brief Main
 *
 * @return int
 */
int main(void)
{
    peripheral_init();  // init basic peripheral, core etc
    ei_usb_init();         // init usb
        
    /* create a task to send data via usb */
    if (xTaskCreate(ei_main_task,
        (const char*) "EI Main Thread",
        EI_MAIN_TASK_STACK_SIZE_BYTE / 4, // in words
        NULL, //pvParameters
        EI_MAIN_TASK_PRIORITY, //uxPriority
        &ei_main_task_handle) != pdPASS) {
        ei_printf("Failed to create EI Main Thread\r\n");
        
        while(1);
    }

    common_event_group = xEventGroupCreate();

    vTaskStartScheduler();
    while (1) {
    };
}

/**
 * @brief 
 * 
 * @param pvParameters 
 */
static void ei_main_task(void *pvParameters)
{
    (void)pvParameters;

    if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        ei_printf("The size of your 'features' array is not correct. Expected %d items, but had %u\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
    }

    ei_impulse_result_t result = { 0 };
    ei_printf("Example standalone - Ambiq Apollo 5");

    while (1) {
        // the features are stored into flash, and we don't want to load everything into RAM
        signal_t features_signal;
        features_signal.total_length = sizeof(features) / sizeof(features[0]);
        features_signal.get_data = &raw_feature_get_data;

        // invoke the impulse
        EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

        if (res != 0) {
            ei_printf("ERR: Failed to run classifier (%d)\n", res);
            vTaskSuspend(NULL);
        }

        ei_printf("Predictions (DSP: %ld us., Classification: %ld us., Anomaly: %ld us.): \n",
                    (int32_t)result.timing.dsp_us, (int32_t)result.timing.classification_us, (int32_t)result.timing.anomaly_us);

        ei_printf("Predictions (DSP: %ld ms., Classification: %ld ms., Anomaly: %ld ms.): \n",
                    (int32_t)result.timing.dsp, (int32_t)result.timing.classification, (int32_t)result.timing.anomaly);

        // print the predictions
        ei_printf("[");

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
        bool bb_found = result.bounding_boxes[0].value > 0;
        for (size_t ix = 0; ix < EI_CLASSIFIER_OBJECT_DETECTION_COUNT; ix++) {
            auto bb = result.bounding_boxes[ix];
            if (bb.value == 0) {
                continue;
            }

            ei_printf("    %s (", bb.label);
            ei_printf_float(bb.value);
            ei_printf(") [ x: %lu, y: %lu, width: %lu, height: %lu ]\n", bb.x, bb.y, bb.width, bb.height);
        }

        if (!bb_found) {
            ei_printf("    No objects found\n");
        }
#else
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            ei_printf("    %s: ",result.classification[ix].label);
            ei_printf_float(result.classification[ix].value);
            ei_printf("\n");
#if EI_CLASSIFIER_HAS_ANOMALY == 1
            ei_printf(", ");
#else
            if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
                ei_printf(", ");
            }
#endif
        }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf_float(result.anomaly);
#endif
        ei_printf("]\n");

        ei_sleep(2000);
    }
}
