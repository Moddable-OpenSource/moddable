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

#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_lpcomp.h"

#define RAM_START_ADDRESS  0x20000000

#define kRamRetentionRegisterMagic 0xBF
#define kRamRetentionBufferSize 256		// must match .retained_section linker size
#define kRamRetentionBufferMagic 0x89341057

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

uint8_t gRamRetentionBuffer[kRamRetentionBufferSize] __attribute__((section(".retained_section"))) = {0};

static uint8_t softdevice_enabled();
static void clear_retained_buffer();
static void lpcomp_event_handler(nrf_lpcomp_event_t event);

/**
	Retention buffer format:
	
	kRamRetentionRegisterMagic
	16-bit buffer length
	buffer
**/

void xs_sleep_set_retained_buffer(xsMachine *the)
{
	uint8_t *buffer = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint16_t bufferLength = xsGetArrayBufferLength(xsArg(0));
	uint8_t *ram = &gRamRetentionBuffer[0];

	uint32_t retainedSize = sizeof(kRamRetentionBufferMagic) + 2 + bufferLength;

	if (retainedSize > kRamRetentionBufferSize)
		xsRangeError("invalid buffer size");

	clear_retained_buffer();
	
	ram[0] = (uint8_t)(kRamRetentionBufferMagic & 0xFF);
	ram[1] = (uint8_t)((kRamRetentionBufferMagic >> 8) & 0xFF);
	ram[2] = (uint8_t)((kRamRetentionBufferMagic >> 16) & 0xFF);
	ram[3] = (uint8_t)((kRamRetentionBufferMagic >> 24) & 0xFF);
	ram += sizeof(kRamRetentionBufferMagic);
	ram[0] = (uint8_t)(bufferLength & 0xFF);
	ram[1] = (uint8_t)((bufferLength >> 8) & 0xFF);
	ram += 2;
	c_memmove(ram, buffer, bufferLength);

	// Eight RAM slave blocks, each divided into two 4 KB sections 
	uint32_t p_offset = (((uint32_t)&gRamRetentionBuffer[0]) - RAM_START_ADDRESS);
	uint8_t ram_slave_n = (p_offset / 8192);  
	uint32_t ram_section_n = (p_offset % 8192) / 4096;
	uint32_t ram_powerset = 1L << (POWER_RAM_POWER_S0RETENTION_Pos + ram_section_n);

	if (softdevice_enabled()) {
		sd_power_gpregret_set(0, kRamRetentionRegisterMagic);
		sd_power_ram_power_set(ram_slave_n, ram_powerset);
	}
	else {
    	NRF_POWER->GPREGRET = kRamRetentionRegisterMagic;
		NRF_POWER->RAM[ram_slave_n].POWERSET = ram_powerset;
	}
}

void xs_sleep_get_retained_buffer(xsMachine *the)
{
	uint8_t *ram;
	uint16_t bufferLength;
	uint32_t gpreg;
	uint8_t sd_enabled = softdevice_enabled();
	
	if (sd_enabled)
		sd_power_gpregret_get(0, &gpreg);
	else
		gpreg = NRF_POWER->GPREGRET;

	// If retention register doesn't contain the magic value there is no retained ram
	if (kRamRetentionRegisterMagic != gpreg)
		return;

	ram = &gRamRetentionBuffer[0];
	if (kRamRetentionBufferMagic != c_read32(ram)) {
		clear_retained_buffer();	// clear retained ram on failure
		return;
	}
	ram += 4;
	bufferLength = c_read16(ram);
	ram += 2;

	xsmcSetArrayBuffer(xsResult, (uint8_t*)ram, bufferLength);
}

void xs_sleep_clear_retained_buffer(xsMachine *the)
{
	clear_retained_buffer();
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

uint8_t softdevice_enabled()
{
#ifdef SOFTDEVICE_PRESENT
	return nrf_sdh_is_enabled();
#else
	return false;
#endif
}

void clear_retained_buffer()
{
	if (softdevice_enabled())
		sd_power_gpregret_set(0, 0);
	else
		NRF_POWER->GPREGRET = 0x00;
	c_memset(&gRamRetentionBuffer[0], 0, kRamRetentionBufferSize);
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

