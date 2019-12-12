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
#include "nrf_sdm.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#define kRamRetentionRegisterMagic 0xBF

#define kRamRetentionBufferSize 256		// must match .retained_section linker size
#define kRamRetentionBufferMagic 0x89341057

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

uint8_t gRamRetentionBuffer[kRamRetentionBufferSize] __attribute__((section(".retained_section"))) = {0};

/**
	Retention buffer format:
	
	kRamRetentionRegisterMagic
	16-bit buffer length
	buffer
**/

void xs_sleep_set_retained_ram_buffer(xsMachine *the)
{
	uint8_t *buffer = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint16_t bufferLength = xsGetArrayBufferLength(xsArg(0));
	uint8_t *ram = &gRamRetentionBuffer[0];
	uint32_t ram_powerset = (POWER_RAM_POWER_S0RETENTION_On << POWER_RAM_POWER_S0RETENTION_Pos);
	uint32_t retainedSize;
	
	retainedSize = sizeof(kRamRetentionBufferMagic) + 2 + bufferLength;

	if (retainedSize > kRamRetentionBufferSize)
		xsRangeError("invalid buffer size");

	c_memset(ram, 0, kRamRetentionBufferSize);
	ram[0] = (uint8_t)(kRamRetentionBufferMagic & 0xFF);
	ram[1] = (uint8_t)((kRamRetentionBufferMagic >> 8) & 0xFF);
	ram[2] = (uint8_t)((kRamRetentionBufferMagic >> 16) & 0xFF);
	ram[3] = (uint8_t)((kRamRetentionBufferMagic >> 24) & 0xFF);
	ram += sizeof(kRamRetentionBufferMagic);
	ram[0] = (uint8_t)bufferLength;
	ram[1] = (uint8_t)(bufferLength >> 8);
	ram += 2;
	c_memmove(ram, buffer, bufferLength);

#ifdef SOFTDEVICE_PRESENT
	uint8_t softdevice_enabled;
	sd_softdevice_is_enabled(&softdevice_enabled);
	if (softdevice_enabled) {
		sd_power_gpregret_set(0, kRamRetentionRegisterMagic);
		sd_power_ram_power_set(0, ram_powerset);
	}
	else
#endif
	{
    	NRF_POWER->GPREGRET = kRamRetentionRegisterMagic;
    	NRF_POWER->RAM[0].POWERSET = ram_powerset;
	}
}

void xs_sleep_get_retained_ram_buffer(xsMachine *the)
{
	uint8_t *ram;
	uint16_t bufferLength;
	uint32_t gpreg;
	uint8_t softdevice_enabled = 0;
	
#ifdef SOFTDEVICE_PRESENT
	sd_softdevice_is_enabled(&softdevice_enabled);
	if (softdevice_enabled) {
		sd_power_gpregret_get(0, &gpreg);
	}
	else
#endif
		gpreg = NRF_POWER->GPREGRET;

	if (kRamRetentionRegisterMagic != gpreg)
		return;

	// Clear general purpose retention register
	if (softdevice_enabled)
		sd_power_gpregret_set(0, 0);
	else
		NRF_POWER->GPREGRET = 0x00;
	
	ram = &gRamRetentionBuffer[0];
	if (kRamRetentionBufferMagic != c_read32(ram))
		xsUnknownError("corrupt retention buffer");
	ram += 4;
	bufferLength = c_read16(ram);
	ram += 2;

	xsmcSetArrayBuffer(xsResult, (uint8_t*)ram, bufferLength);
}

// Set the power mode for System On sleep
void xs_sleep_set_power_mode(xsMachine *the)
{
	uint16_t mode = xsmcToInteger(xsArg(0));

	if (!(mode == kPowerModeConstantLatency || mode == kPowerModeLowPower))
		xsUnknownError("invalid power mode");
		
#ifdef SOFTDEVICE_PRESENT
	uint8_t softdevice_enabled;
	sd_softdevice_is_enabled(&softdevice_enabled);
	if (softdevice_enabled)
		sd_power_mode_set(kPowerModeConstantLatency == mode ? NRF_POWER_MODE_CONSTLAT : NRF_POWER_MODE_LOWPWR);
	else
#endif
		if (kPowerModeConstantLatency == mode)
			NRF_POWER->TASKS_CONSTLAT = 1;
		else if (kPowerModeLowPower == mode)
			NRF_POWER->TASKS_LOWPWR = 1;
}

void xs_sleep_get_power_mode(xsMachine *the)
{
	uint16_t mode;
	
#ifdef SOFTDEVICE_PRESENT
	uint8_t softdevice_enabled;
	sd_softdevice_is_enabled(&softdevice_enabled);
	if (softdevice_enabled)
		mode = kPowerModeUnknown;	// @@ doesn't seem to be a way to know power mode when SoftDevice is enabled??
	else
#endif
		if (1 == NRF_POWER->TASKS_CONSTLAT)
			mode = kPowerModeConstantLatency;
		else if (1 == NRF_POWER->TASKS_LOWPWR)
			mode = kPowerModeLowPower;
		else
			mode = kPowerModeUnknown;
			
	xsmcSetInteger(xsResult, mode);
}

void xs_sleep_deep(xsMachine *the)
{
#ifdef SOFTDEVICE_PRESENT
	uint8_t softdevice_enabled;
	sd_softdevice_is_enabled(&softdevice_enabled);
	if (softdevice_enabled)
		sd_power_system_off();
	else
#endif
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
#ifdef mxDebug
	for (;;) {}
#endif
}

void xs_sleep_get_reset_reason(xsMachine *the)
{
	uint32_t reset_reason;
	uint8_t softdevice_enabled = 0;
	
#ifdef SOFTDEVICE_PRESENT
	sd_softdevice_is_enabled(&softdevice_enabled);
	if (softdevice_enabled)
		sd_power_reset_reason_get(&reset_reason);
	else
#endif
		reset_reason = NRF_POWER->RESETREAS;

	// clear the reset reason register using the bit mask
	if (softdevice_enabled)
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
    nrf_delay_ms(10);
}

void xs_sleep_wake_on_analog(xsMachine *the)
{
	uint16_t pin = xsmcToInteger(xsArg(0));

}
