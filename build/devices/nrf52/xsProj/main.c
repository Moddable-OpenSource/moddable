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

#include <stdint.h>
#include <string.h>
#include "app_error.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_wdt.h"
#include "xsHost.h"
#include "FreeRTOS.h"
#include "task.h"

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
}

static void timer_init(void)
{
	ret_code_t err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);
}

#if !mxDebug
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
	ret_code_t err_code = nrf_drv_wdt_init(&config, wdt_event_handler);
	APP_ERROR_CHECK(err_code);
	nrf_drv_wdt_channel_alloc(&wdt_channel_id);
	nrf_drv_wdt_enable();
}
#endif

/**@brief Function for application main entry.
 */
void xs_setup();
void xs_main();

int main(void)
{
    // Initialize modules.
	clock_init();
	timer_init();
#if !mxDebug
	watchdog_init();
#endif

    // Activate deep sleep mode.
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

#if NRF_LOG_ENABLED
    if (pdPASS != xTaskCreate(logger_thread, "LOGGER", 256, NULL, 1, &m_logger_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
#endif

    xs_setup();
   
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

// Implementation of Nordic vPortSuppressTicksAndSleep() function that guards every SoftDevice API call.
// If the SoftDevice is not enabled, the code uses the non-SoftDevice APIs.
// https://devzone.nordicsemi.com/f/nordic-q-a/55486/gpio-wakeup-from-system-off-under-freertos-restarts-at-address-0x00000a80
// https://www.freertos.org/low-power-tickless-rtos.html

#if configUSE_TICKLESS_IDLE == 1

void vApplicationSleep(TickType_t xExpectedIdleTime)
{
    /*
     * Implementation note:
     *
     * To help debugging the option configUSE_TICKLESS_IDLE_SIMPLE_DEBUG was presented.
     * This option would make sure that even if program execution was stopped inside
     * this function no more than expected number of ticks would be skipped.
     *
     * Normally RTC works all the time even if firmware execution was stopped
     * and that may lead to skipping too much of ticks.
     */
    TickType_t enterTime;

    /* Make sure the SysTick reload value does not overflow the counter. */
    if ( xExpectedIdleTime > portNRF_RTC_MAXTICKS - configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
    {
        xExpectedIdleTime = portNRF_RTC_MAXTICKS - configEXPECTED_IDLE_TIME_BEFORE_SLEEP;
    }
    /* Block all the interrupts globally */
#ifdef SOFTDEVICE_PRESENT
	if (nrf_sdh_is_enabled()) {
		do{
			uint8_t dummy = 0;
			uint32_t err_code = sd_nvic_critical_region_enter(&dummy);
			APP_ERROR_CHECK(err_code);
		}while (0);
	}
	else
#endif
		__disable_irq();

    enterTime = nrf_rtc_counter_get(portNRF_RTC_REG);

    if ( eTaskConfirmSleepModeStatus() != eAbortSleep )
    {
        TickType_t xModifiableIdleTime;
        TickType_t wakeupTime = (enterTime + xExpectedIdleTime) & portNRF_RTC_MAXTICKS;

        /* Stop tick events */
        nrf_rtc_int_disable(portNRF_RTC_REG, NRF_RTC_INT_TICK_MASK);

        /* Configure CTC interrupt */
        nrf_rtc_cc_set(portNRF_RTC_REG, 0, wakeupTime);
        nrf_rtc_event_clear(portNRF_RTC_REG, NRF_RTC_EVENT_COMPARE_0);
        nrf_rtc_int_enable(portNRF_RTC_REG, NRF_RTC_INT_COMPARE0_MASK);

        __DSB();

        /* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
         * set its parameter to 0 to indicate that its implementation contains
         * its own wait for interrupt or wait for event instruction, and so wfi
         * should not be executed again.  However, the original expected idle
         * time variable must remain unmodified, so a copy is taken. */
        xModifiableIdleTime = xExpectedIdleTime;
        configPRE_SLEEP_PROCESSING( xModifiableIdleTime );
        if ( xModifiableIdleTime > 0 )
        {
#ifdef SOFTDEVICE_PRESENT
            if (nrf_sdh_is_enabled())
            {
                uint32_t err_code = sd_app_evt_wait();
                APP_ERROR_CHECK(err_code);
            }
            else
#endif
            {
                /* No SD -  we would just block interrupts globally.
                * BASEPRI cannot be used for that because it would prevent WFE from wake up.
                */
                do{
                    __WFE();
                } while (0 == (NVIC->ISPR[0] | NVIC->ISPR[1]));
            }
        }
        configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

        nrf_rtc_int_disable(portNRF_RTC_REG, NRF_RTC_INT_COMPARE0_MASK);
        nrf_rtc_event_clear(portNRF_RTC_REG, NRF_RTC_EVENT_COMPARE_0);

        /* Correct the system ticks */
        {
            TickType_t diff;
            TickType_t exitTime;

            nrf_rtc_event_clear(portNRF_RTC_REG, NRF_RTC_EVENT_TICK);
            nrf_rtc_int_enable (portNRF_RTC_REG, NRF_RTC_INT_TICK_MASK);

            exitTime = nrf_rtc_counter_get(portNRF_RTC_REG);
            diff =  (exitTime - enterTime) & portNRF_RTC_MAXTICKS;

            /* It is important that we clear pending here so that our corrections are latest and in sync with tick_interrupt handler */
            NVIC_ClearPendingIRQ(portNRF_RTC_IRQn);

            if ((configUSE_TICKLESS_IDLE_SIMPLE_DEBUG) && (diff > xExpectedIdleTime))
            {
                diff = xExpectedIdleTime;
            }

            if (diff > 0)
            {
                vTaskStepTick(diff);
            }
        }
    }
#ifdef SOFTDEVICE_PRESENT
	if (nrf_sdh_is_enabled()) {
		uint32_t err_code = sd_nvic_critical_region_exit(0);
		APP_ERROR_CHECK(err_code);
	}
	else
#endif
		__enable_irq();
}

#endif
