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

#include "peripheral.h"
#include "ns_core.h"
#include "ns_malloc.h"
#include "ns_peripherals_power.h"
#include "am_devices_led.h"
#include "am_mcu_apollo.h"
#include "am_bsp.h"

/// NeuralSPOT Includes
#include "ns_ambiqsuite_harness.h"

const ns_power_config_t ns_pwr_config = {
    .api = &ns_power_V1_0_0,
    .eAIPowerMode = NS_MAXIMUM_PERF,
    .bNeedAudAdc = false,
    .bNeedSharedSRAM = true,
    .bNeedCrypto = false,
    .bNeedBluetooth = false,
    .bNeedUSB = true,
    .bNeedIOM = false,
    .bNeedAlternativeUART = false,
    .b128kTCM = false,
    //.bNeedXtal = true,
    };

ns_timer_config_t ei_tickTimer = {
    .api = &ns_timer_V1_0_0,
    .timer = NS_TIMER_COUNTER,
    .enableInterrupt = false,
};

void peripheral_init(void)
{
    ns_core_config_t ns_core_cfg = {.api = &ns_core_V1_0_0};

    // NeuralSPOT inits
    NS_TRY(ns_core_init(&ns_core_cfg), "Core init failed.\n");

        //
    // Enable the I-Cache and D-Cache.
    //
    am_hal_cachectrl_icache_enable();
    am_hal_cachectrl_dcache_enable(true);

#ifdef ENERGY_MONITOR_ENABLE
    // This is for measuring power using an external power monitor such as
    // Joulescope - it sets GPIO pins so the state can be observed externally
    // to help line up the waveforms. It has nothing to do with AI...
    ns_init_power_monitor_state();
#endif
    //ns_set_power_monitor_state(NS_IDLE); // no-op if ns_init_power_monitor_state not called
    NS_TRY(ns_power_config(&ns_pwr_config), "Power Init Failed.\n");
    am_hal_pwrctrl_mcu_mode_select(AM_HAL_PWRCTRL_MCU_MODE_HIGH_PERFORMANCE);

    // Pick either ns_uart_printf_enable or ns_itm_printf_enable dependin on your needs
    //ns_uart_printf_enable(); // use uart to print, uses less power
    //ns_itm_printf_enable();
    /* A note about printf and low power: printing over ITM impacts low power in two
        ways:
        1) enabling ITM prevents SoC from entering deep sleep, and
        2) ITM initialization requires crypto to be enabled.

        While ITM printing isn't something a deployed application would enable, NS does the
        following to mitigate power usage during ITM:

        1) ns_power_config() lets you set bNeedITM
        2) ns_itm_printf_enable() will temporarily enable crypto if needed
        3) ns_lp_printf() enables TPIU and ITM when needed
        4) ns_deep_sleep() disables crypto, TPIU and ITM if enabled to allow full deep sleep
    */

#ifdef NS_MLPROFILE
    NS_TRY(ns_timer_init(&basic_tickTimer), "Timer init failed.\n");
#endif

    ns_malloc_init(); // needed by EI
    NS_TRY(ns_timer_init(&ei_tickTimer), "Timer init failed.\n");
    ns_interrupt_master_enable();

    // Initialize the LED
    //am_devices_led_init(am_bsp_psLEDs);
}
