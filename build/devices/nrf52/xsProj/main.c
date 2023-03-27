/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Modified 2020-2023 for Moddable Tech, Inc.
 */

#include <stdint.h>
#include <string.h>
#include "app_error.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_wdt.h"
#include "xsHost.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ftdi_trace.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_rtc.h"
#endif

#if NRF_LOG_ENABLED
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static TaskHandle_t m_logger_thread;
#endif

#include "nrf_drv_power.h"

#include "xsmain.h"

/**@brief A function which is hooked to idle task.
 * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
 */
void vApplicationIdleHook( void )
{
#if NRF_LOG_ENABLED
	vTaskResume(m_logger_thread);
#endif
}

#if NRF_LOG_ENABLED
/**@brief Thread for flushing deferred writes to the logger.
 */
static void logger_thread(void * arg)
{
    UNUSED_PARAMETER(arg);

    while (1)
    {
        NRF_LOG_FLUSH();

        vTaskSuspend(NULL); // Suspend myself
    }
}
#endif //NRF_LOG_ENABLED

static void clock_init(void)
{
	ret_code_t err_code = nrf_drv_clock_init();
	APP_ERROR_CHECK(err_code);
	NRF_POWER->TASKS_CONSTLAT = 1;
}

static void timer_init(void)
{
	ret_code_t err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);
}

#if USE_WATCHDOG
	static nrf_drv_wdt_channel_id wdt_channel_id;

	static void wdt_event_handler(void)
	{
		// Watchdog expired.
		// System RESET!!!
		// NOTE: The max amount of time we can spend in WDT interrupt is two cycles of 32768[Hz] clock - after that, reset occurs
	}

	static void watchdog_init(void)
	{
		nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;

		nrf_drv_clock_lfclk_request(NULL);

		// If the WDT is enabled (but not expired), after a soft reset (NVIC_SystemReset) the RTC no longer runs.
		// This scenario occurs when using the RTC to wakeup from light sleep (wake on timer).
		// The workaround is to start the low frequency clock here before initializing the WDT driver.
		// Reference: https://devzone.nordicsemi.com/f/nordic-q-a/65361/rtc-stops-running-after-soft-reset-with-wdt-enabled
		nrfx_clock_lfclk_start();

		ret_code_t err_code = nrf_drv_wdt_init(&config, wdt_event_handler);
		APP_ERROR_CHECK(err_code);
		NRF_WDT->CONFIG = 0;	// pause on sleep and halt
		nrf_drv_wdt_channel_alloc(&wdt_channel_id);
		nrf_drv_wdt_enable();
	}
#else
	#define watchdog_init()
#endif

/**@brief Function for application main entry.
 */

int main(void)
{
	clock_init();
	timer_init();

#ifdef mxDebug
	nrf_drv_power_init(NULL);
#endif

	// Grab the reset reason early. Because the reset reason register is cumulative, clear it now.
	nrf52_set_reset_reason(NRF_POWER->RESETREAS);
	if (MOD_GPIO_WAKE_MAGIC == ((uint32_t *)MOD_WAKEUP_REASON_MEM)[0])
		nrf52_set_boot_latches(((uint32_t *)MOD_WAKEUP_REASON_MEM)[1], ((uint32_t *)MOD_WAKEUP_REASON_MEM)[2]);
	else
		nrf52_set_boot_latches(NRF_P0->LATCH, NRF_P1->LATCH);
	NRF_P0->DETECTMODE = 0;
	NRF_P1->DETECTMODE = 0;
	NRF_P0->LATCH = 0xFFFFFFFF;
	NRF_P1->LATCH = 0xFFFFFFFF;

//	NRF_POWER->RESETREAS = 0xFFFFFFFF;
	NRF_POWER->RESETREAS = NRF_POWER->RESETREAS;	// A field is cleared by writing `1` to it

	watchdog_init();

	// Activate deep sleep mode.
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

#if NRF_LOG_ENABLED
	if (pdPASS != xTaskCreate(logger_thread, "LOGGER", 256, NULL, 1, &m_logger_thread))
	{
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}
#endif

	xs_setup();
	vTaskStartScheduler();

    for (;;)
    {
		APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
    }
}

void vApplicationMallocFailedHook( void )
{
}

void vApplicationStackOverflowHook( void )
{
}

#if configSUPPORT_STATIC_ALLOCATION

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
	static StaticTask_t xTimerTaskTCB;
	static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif
