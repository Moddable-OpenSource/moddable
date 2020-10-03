/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsmc.h"
#include "xsHost.h"
#include "xsPlatform.h"

#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_lpcomp.h"
#include "nrf_rtc.h"
#include "FreeRTOS.h"

#define RAM_START_ADDRESS  0x20000000
#define kRamRetentionBufferSize 256		// must match .retained_section/.no_init linker size 0x100

#define kRamRetentionRegisterMagic 0xBF
#define kRamRetentionBufferMagic 0x89341057

#define kRamRetentionValueMagic 0x52081543

#define kAnalogResolution 10	// 10 bits

enum {
	kPowerModeUnknown = 0,
	kPowerModeConstantLatency,
	kPowerModeLowPower
};

enum {
	kSleepModeUnknown = 0,
	kSleepModeSystemOn,
	kSleepModeSystemOff
};

enum {
	kResetReasonUnknown = 0,
	kResetReasonResetPin = (1L << 0),
	kResetReasonWatchdog = (1L << 1),
	kResetReasonSoftReset = (1L << 2),
	kResetReasonLockup = (1L << 3),
	kResetReasonGPIO = (1L << 16),
	kResetReasonLPCOMP = (1L << 17),
	kResetReasonDebugInterface = (1L << 18),
	kResetReasonNFC = (1L << 19)
};

enum {
	kAnalogWakeModeUnknown = -1,
	kAnalogWakeModeCrossing,
	kAnalogWakeModeUp,
	kAnalogWakeModeDown
};

#ifdef MODGCC
	uint8_t gRamRetentionBuffer[kRamRetentionBufferSize] __attribute__((section(".no_init"))) __attribute__((used)) = {0};
#else
	uint8_t gRamRetentionBuffer[kRamRetentionBufferSize] __attribute__((section(".retained_section"))) = {0};
#endif

typedef struct modSleepHandlerRecord modSleepHandlerRecord;
typedef modSleepHandlerRecord *modSleepHandler;

struct modSleepHandlerRecord {
	struct modSleepHandlerRecord *next;
	xsSlot callback;
};

typedef struct {
	uint8_t suspended;
	uint8_t pending;
	modSleepHandler handlers;
} modSleepRecord, *modSleep;

static modSleepRecord gSleep = {0};

static uint8_t softdevice_enabled();
static void sleep_deep(xsMachine *the);
static void lpcomp_event_handler(nrf_lpcomp_event_t event);
static void getRAMSlaveAndSection(uint32_t address, uint32_t *slave, uint32_t *section);

void xs_sleep_install(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	modSleepHandler handler;
	if (0 == argc) {
		while (NULL != (handler = gSleep.handlers)) {
			gSleep.handlers = handler->next;
			xsForget(handler->callback);
			c_free(handler);
		}
	}
	else {
		handler = c_malloc(sizeof(modSleepHandlerRecord));
		handler->next = NULL;
		handler->callback = xsArg(0);
		xsRemember(handler->callback);
		handler->next = gSleep.handlers;
		gSleep.handlers = handler;
	}
}

void xs_sleep_prevent(xsMachine *the)
{
	++gSleep.suspended;
}

void xs_sleep_allow(xsMachine *the)
{
	--gSleep.suspended;
	if (gSleep.suspended <= 0) {
		gSleep.suspended = 0;
		if (gSleep.pending)
			sleep_deep(the);
	}
}

/**
	Retained value format (each value requires 8 bytes):
		- 32-bit kRamRetentionValueMagic
		- 32-bit value
**/

void xs_sleep_clear_retained_value(xsMachine *the)
{
	int16_t index = xsmcToInteger(xsArg(0));
	uint32_t *slots = (uint32_t*)&gRamRetentionBuffer[0];
	
	if (index < 0 || index > 31)
		xsRangeError("invalid index");
	
	slots[index * 2] = 0;
}

void xs_sleep_get_retained_value(xsMachine *the)
{
	int16_t index = xsmcToInteger(xsArg(0));
	int32_t *slots = (uint32_t*)&gRamRetentionBuffer[0];
	uint32_t gpreg;
	
	if (index < 0 || index > 31)
		xsRangeError("invalid index");

	if (softdevice_enabled())
		sd_power_gpregret_get(0, &gpreg);
	else
		gpreg = NRF_POWER->GPREGRET;

	// If retention register doesn't contain the magic value there is no retained ram
	if (kRamRetentionRegisterMagic != gpreg)
		return;

	if (kRamRetentionValueMagic == slots[index * 2])
		xsResult = xsInteger(slots[(index * 2) + 1]);
}

void xs_sleep_set_retained_value(xsMachine *the)
{
	int16_t index = xsmcToInteger(xsArg(0));
	int32_t value = xsmcToInteger(xsArg(1));
	int32_t *slots = (uint32_t*)&gRamRetentionBuffer[0];
	uint32_t ram_slave, ram_section, ram_powerset;
	
	if (index < 0 || index > 31)
		xsRangeError("invalid index");
	
	getRAMSlaveAndSection((uint32_t)&gRamRetentionBuffer[0], &ram_slave, &ram_section);

	ram_powerset = (1L << (POWER_RAM_POWER_S0RETENTION_Pos + ram_section)) |
		(1L << (POWER_RAM_POWER_S0POWER_Pos + ram_section));

	if (softdevice_enabled()) {
		sd_power_gpregret_set(0, kRamRetentionRegisterMagic);
		sd_power_ram_power_set(ram_slave, ram_powerset);
	}
	else {
    	NRF_POWER->GPREGRET = kRamRetentionRegisterMagic;
		NRF_POWER->RAM[ram_slave].POWERSET = ram_powerset;
	}

	slots[index * 2] = kRamRetentionValueMagic;
	slots[(index * 2) + 1] = value;
	
	nrf_delay_ms(1);
}

// Set the power mode for System On sleep
void xs_sleep_set_power_mode(xsMachine *the)
{
	uint16_t mode = xsmcToInteger(xsArg(0));

	if (!(mode == kPowerModeConstantLatency || mode == kPowerModeLowPower))
		xsUnknownError("invalid power mode");
		
	if (softdevice_enabled())
		sd_power_mode_set(kPowerModeConstantLatency == mode ? NRF_POWER_MODE_CONSTLAT : NRF_POWER_MODE_LOWPWR);
	else {
		if (kPowerModeConstantLatency == mode)
			NRF_POWER->TASKS_CONSTLAT = 1;
		else if (kPowerModeLowPower == mode)
			NRF_POWER->TASKS_LOWPWR = 1;
	}
}

void xs_sleep_deep(xsMachine *the)
{
#ifdef mxDebug
	modLog("Deep sleep only supported for release builds!");
	return;
#endif

	if (gSleep.suspended > 0) {
		gSleep.pending = true;
		return;
	}
	
	sleep_deep(the);
}

void xs_sleep_get_reset_reason(xsMachine *the)
{
	uint32_t reset_reason;
	uint8_t sd_enabled = softdevice_enabled();

	if (sd_enabled)
		sd_power_reset_reason_get(&reset_reason);
	else
		reset_reason = NRF_POWER->RESETREAS;

	// clear the reset reason register using the bit mask
	if (sd_enabled)
		sd_power_reset_reason_clr(reset_reason);
	else
		NRF_POWER->RESETREAS = reset_reason;
		
	xsmcSetInteger(xsResult, reset_reason);	
}

void xs_sleep_get_reset_pin(xsMachine *the)
{
	uint32_t i;
	int32_t result = -1;
	uint32_t pins = NRF_GPIO->IN;
	
	for (i = 0; i < 32; ++i) {
		if (pins & (1L << i)) {
			result = i;
			break;
		}
	}
	
	xsmcSetInteger(xsResult, result);	
}

void xs_sleep_wake_on_digital(xsMachine *the)
{
	uint16_t pin = xsmcToInteger(xsArg(0));
	
	nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
	
    // Workaround for PAN_028 rev1.1 anomaly 22 - System: Issues with disable System OFF mechanism
    nrf_delay_ms(1);
}

void xs_sleep_wake_on_analog(xsMachine *the)
{
	uint16_t input = xsmcToInteger(xsArg(0));
	uint16_t detection = xsmcToInteger(xsArg(1));
	uint16_t value = xsmcToInteger(xsArg(2));
	nrf_drv_lpcomp_config_t config;
	uint16_t reference;
	double scaledValue;
	ret_code_t err_code;

	if (input < NRF_LPCOMP_INPUT_0 || input > NRF_LPCOMP_INPUT_7)
		xsRangeError("invalid analog channel number");

	if (detection < kAnalogWakeModeCrossing || detection > kAnalogWakeModeDown)
		xsRangeError("invalid analog detect mode");

	scaledValue = ((double)value) / (1L << kAnalogResolution);
	reference = (uint16_t)(scaledValue * (LPCOMP_REFSEL_REFSEL_SupplySevenEighthsPrescaling - LPCOMP_REFSEL_REFSEL_SupplyOneEighthPrescaling + 1));

	config.hal.reference = reference;
	config.hal.detection = detection;
	config.hal.hyst = 0;
	config.input = input;
	config.interrupt_priority = 6;
	err_code = nrf_drv_lpcomp_init(&config, lpcomp_event_handler);
	if (NRF_SUCCESS != err_code)
		xsUnknownError("wake on analog config failure");

	nrf_drv_lpcomp_enable();

	nrf_delay_ms(10);	// @@ seems necessary?
}

void xs_sleep_wake_on_interrupt(xsMachine *the)
{
	uint16_t pin = xsmcToInteger(xsArg(0));

	nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);

	// Workaround for PAN_028 rev1.1 anomaly 22 - System: Issues with disable System OFF mechanism
	nrf_delay_ms(1);
}

void xs_sleep_wake_on_timer(xsMachine *the)
{
	uint32_t ms = xsmcToInteger(xsArg(0));
    TickType_t enterTime, exitTime, wakeupTime;
    uint32_t ram_slave, ram_section, ram_powerset;

	getRAMSlaveAndSection((uint32_t)&gRamRetentionBuffer[0], &ram_slave, &ram_section);

    /* Block all the interrupts globally */
    if (softdevice_enabled()) {
		do {
			uint8_t dummy = 0;
			uint32_t err_code = sd_nvic_critical_region_enter(&dummy);
			APP_ERROR_CHECK(err_code);
		} while (0);
    }
    else {
		__disable_irq();
    }

	// @@ read our clock and save on stack
	
    // calculate wake up time
    enterTime = nrf_rtc_counter_get(portNRF_RTC_REG);
	wakeupTime = (enterTime + ms) & portNRF_RTC_MAXTICKS;

	// stop tick events
	nrf_rtc_int_disable(portNRF_RTC_REG, NRF_RTC_INT_TICK_MASK);

	// configure CTC interrupt
	nrf_rtc_cc_set(portNRF_RTC_REG, 0, wakeupTime);
	nrf_rtc_event_clear(portNRF_RTC_REG, NRF_RTC_EVENT_COMPARE_0);
	nrf_rtc_int_enable(portNRF_RTC_REG, NRF_RTC_INT_COMPARE0_MASK);

    __DSB();
    
	// power off all ram sections except for stack and ram retention
	// note that slaves 0-7 have two 4 KB sections and slave 8 has six 32 KB sections
	if (softdevice_enabled()) {
		sd_power_ram_power_clr(0, 0x03);
		sd_power_ram_power_clr(1, 0x03);
		sd_power_ram_power_clr(2, 0x02);
		sd_power_ram_power_clr(3, 0x03);
		sd_power_ram_power_clr(4, 0x03);
		sd_power_ram_power_clr(5, 0x03);
		sd_power_ram_power_clr(6, 0x03);
		sd_power_ram_power_clr(7, 0x03);
		sd_power_ram_power_clr(8, 0x1F);
	}
	else {
//		NRF_POWER->RAM[0].POWERCLR = 0x03;	// sections 0-1
//		NRF_POWER->RAM[1].POWERCLR = 0x03;
//		NRF_POWER->RAM[2].POWERCLR = 0x02;	// section 1
//		NRF_POWER->RAM[3].POWERCLR = 0x03;
//		NRF_POWER->RAM[4].POWERCLR = 0x03;
//		NRF_POWER->RAM[5].POWERCLR = 0x03;
		NRF_POWER->RAM[6].POWERCLR = 0x03;
//		NRF_POWER->RAM[7].POWERCLR = 0x03;
//		NRF_POWER->RAM[8].POWERCLR = 0x1F;	// sections 0-4, stack lives in section 5 at top of memory
	}
		
	// keep power on ram retention section
	ram_powerset = (1L << (POWER_RAM_POWER_S0RETENTION_Pos + ram_section)) |
		(1L << (POWER_RAM_POWER_S0POWER_Pos + ram_section));

	if (softdevice_enabled()) {
		sd_power_gpregret_set(0, kRamRetentionRegisterMagic);
		sd_power_ram_power_set(ram_slave, ram_powerset);
	}
	else {
    	NRF_POWER->GPREGRET = kRamRetentionRegisterMagic;
		NRF_POWER->RAM[ram_slave].POWERSET = ram_powerset;
	}

	// wait for event - hopefully the RTC interrupt
	__SEV();
	__WFE();
	__WFE();

	// @@ retain clock value based on the amount of time we slept and the clock value saved on stack

	nrf52_reset();
}

void sleep_deep(xsMachine *the)
{
	modSleepHandler walker;

	walker = gSleep.handlers;
	while (NULL != walker) {
		xsCallFunction0(walker->callback, xsGlobal);
		walker = walker->next;
	}

	if (softdevice_enabled())
		sd_power_system_off();
	else
		NRF_POWER->SYSTEMOFF = 1;

	// Use data synchronization barrier and a delay to ensure that no failure
	// indication occurs before System OFF is actually entered.
	__DSB();
	__NOP();

	// System Off mode is emulated in debug mode. It is therefore suggested to include an
	// infinite loop right after System OFF to prevent the CPU from executing code that shouldn't
	// normally be executed. 
	// https://devzone.nordicsemi.com/f/nordic-q-a/55486/gpio-wakeup-from-system-off-under-freertos-restarts-at-address-0x00000a80
	// https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52840%2Fpower.html&cp=4_0_0_4_2_2_0&anchor=unique_142049681

	// Note that even with this loop, on debug builds, the application ends up in what appears to be 
	// the Hardfault_Handler at restart after a wakeup from a digital pin.
	// This code should never be reached on release builds.
#ifdef mxDebug
	for (;;) {}
#endif
}

uint8_t softdevice_enabled()
{
#ifdef SOFTDEVICE_PRESENT
	return nrf_sdh_is_enabled();
#else
	return false;
#endif
}

void lpcomp_event_handler(nrf_lpcomp_event_t event)
{
	switch(event) {
		case NRF_LPCOMP_EVENT_READY:
			break;
		case NRF_LPCOMP_EVENT_DOWN:
			break;
		case NRF_LPCOMP_EVENT_UP:
			break;
		case NRF_LPCOMP_EVENT_CROSS:
			break;
	}
}

void getRAMSlaveAndSection(uint32_t address, uint32_t *slave, uint32_t *section)
{
	// From the nRF52840 Product Specification:
	// The RAM interface is divided into 9 RAM AHB slaves.
	// RAM AHB slave 0-7 is connected to 2x4 kB RAM sections each and RAM AHB slave 8 is connected to 6x32 kB sections
	uint32_t p_offset = address - RAM_START_ADDRESS;
	if (address < 0x20010000L) {
		*slave = (p_offset / 8192);  
		*section = (p_offset % 8192) / 4096;
	}
	else {
		*slave = 8;
		address -= 0x20010000L;
		*section = address / 32768L;
	}
}

