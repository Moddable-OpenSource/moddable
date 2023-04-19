/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
#include "xsPlatform.h"
#include "modGPIO.h"
#include "mc.defines.h"

#include "nrf_drv_clock.h"
#include "nrf_drv_lpcomp.h"
#include "nrf_sdh.h"
#include "nrf_rtc.h"

#ifndef MODDEF_SLEEP_FASTRESET
	#define MODDEF_SLEEP_FASTRESET (false)
#endif

#define RAM_START_ADDRESS  0x20000000
#define kRamRetentionBufferSize 256		// must match .retained_section/.no_init linker size 0x100

#define kRamRetentionValueMagic 0x52081543
#define kRamRetentionSlots 32

enum {
	kPowerModeUnknown = 0,
	kPowerModeConstantLatency,
	kPowerModeLowPower
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

#ifdef MODGCC
	uint8_t gRamRetentionBuffer[kRamRetentionBufferSize] __attribute__((section(".no_init"))) __attribute__((used)) = {0};
#else
	uint8_t gRamRetentionBuffer[kRamRetentionBufferSize] __attribute__((section(".retained_section"))) = {0};
#endif

static void getRAMSlaveAndSection(uint32_t address, uint32_t *slave, uint32_t *section);

static void sleep_wake_on_timer();
static void sleepPinsOff();

// The gRamRetentionBuffer contains 32-bit retained value slots
// When values are retained, the slot contents are as follows:
//    slot[0]: kRamRetentionValueMagic
//    slot[1] - slot[32]: Retained values for slot indexes 0 to 31

void xs_sleep_get_retained_value(xsMachine *the)
{
	int32_t index = xsmcToInteger(xsArg(0));
	uint32_t *slots = (uint32_t*)&gRamRetentionBuffer[0];
	
	if (index < 0 || index > 31)
		xsRangeError("invalid index");

	if (kRamRetentionValueMagic != slots[0])
		c_memset(gRamRetentionBuffer, 0, sizeof(gRamRetentionBuffer));

	xsResult = xsInteger(slots[index + 1]);
}

void xs_sleep_set_retained_value(xsMachine *the)
{
	int32_t index = xsmcToInteger(xsArg(0));
	int32_t value = xsmcToInteger(xsArg(1));
	uint32_t *slots = (uint32_t*)&gRamRetentionBuffer[0];
	uint32_t ram_slave, ram_section, ram_powerset;
	
	if (index < 0 || index > 31)
		xsRangeError("invalid index");
	
	getRAMSlaveAndSection((uint32_t)&gRamRetentionBuffer[0], &ram_slave, &ram_section);

	ram_powerset = (1L << (POWER_RAM_POWER_S0RETENTION_Pos + ram_section)) |
		(1L << (POWER_RAM_POWER_S0POWER_Pos + ram_section));

	if (nrf52_softdevice_enabled())
		sd_power_ram_power_set(ram_slave, ram_powerset);
	else
		NRF_POWER->RAM[ram_slave].POWERSET = ram_powerset;

	if (kRamRetentionValueMagic != slots[0])
		c_memset(gRamRetentionBuffer, 0, sizeof(gRamRetentionBuffer));

	slots[0] = kRamRetentionValueMagic;
	slots[index + 1] = value;
	
	nrf_delay_ms(1);
}

// Set the power mode for System On sleep
void xs_sleep_set_power_mode(xsMachine *the)
{
	uint16_t mode = xsmcToInteger(xsArg(0));

	if (!(mode == kPowerModeConstantLatency || mode == kPowerModeLowPower))
		xsUnknownError("invalid power mode");
		
	if (nrf52_softdevice_enabled())
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
	modRunOnSleepCallbacks();

#ifdef mxDebug
	modLog("Deep sleep only supported for release builds!");
	return;
#endif

	sleepPinsOff();

	uint16_t argc = xsmcArgc;
	if (argc > 0) {
		// System ON sleep, RAM powered off during sleep, wake on RTC or configured interrupt source
		
		uint32_t ms = xsmcToInteger(xsArg(0));
		uint32_t waitTicks = (((uint64_t)ms) << 10) / 1000;
		
		// Stop tick events
		nrf_rtc_int_disable(portNRF_RTC_REG, NRF_RTC_INT_TICK_MASK);

		// Block all the interrupts globally
		if (nrf52_softdevice_enabled()) {
			do {
				uint8_t dummy = 0;
				uint32_t err_code = sd_nvic_critical_region_enter(&dummy);
				APP_ERROR_CHECK(err_code);
			} while (0);
		}
		else
			__disable_irq();

		// Save time of day in retention memory
		c_timeval tv;
		c_gettimeofday(&tv, NULL);
		*((c_timeval *)MOD_TIME_RESTORE_MEM) = tv;

		// Disable LFCLK started interrupt
		nrf_clock_int_disable(CLOCK_INTENSET_LFCLKSTARTED_Msk);
                          
		// Disable SoftDevice
		if (nrf52_softdevice_enabled())
			nrf_sdh_disable_request();
			
		// Stop the RTC and clear the RTC counter
		nrf_rtc_task_trigger(portNRF_RTC_REG, NRF_RTC_TASK_STOP);
		nrf_rtc_task_trigger(portNRF_RTC_REG, NRF_RTC_TASK_CLEAR);

		// Clear GPIO P0 latch register to clear any pending digital input pins
		NRF_P0->LATCH = 0xFFFFFFFF;
		NRF_P1->LATCH = 0xFFFFFFFF;
		
		// Enable LPCOMP if configured
		if (NRF_LPCOMP->INTENSET & (LPCOMP_INTENSET_UP_Msk | LPCOMP_INTENSET_DOWN_Msk | LPCOMP_INTENSET_CROSS_Msk)) {
			nrf_drv_lpcomp_enable();
			nrf_delay_ms(10);
		}
		
		// Configure CTC interrupt
		nrf_rtc_cc_set(portNRF_RTC_REG, 0, waitTicks);
		nrf_rtc_event_clear(portNRF_RTC_REG, NRF_RTC_EVENT_COMPARE_0);
		nrf_rtc_int_enable(portNRF_RTC_REG, NRF_RTC_INT_COMPARE0_MASK);

		// Start the RTC
		nrf_rtc_task_trigger(portNRF_RTC_REG, NRF_RTC_TASK_START);
		
		// Ensure memory access has completed
		__DSB();

		// Wake on timer
		sleep_wake_on_timer();		
	}
	else {
		// System OFF sleep, wake on reset or preconfigured analog/digital wake-up trigger

#if MODDEF_SLEEP_FASTRESET
		*((uint32_t*)DFU_DBL_RESET_MEM) = REBOOT_FAST_RESET;
#endif

		// Enable LPCOMP if configured
		if (NRF_LPCOMP->INTENSET & (LPCOMP_INTENSET_UP_Msk | LPCOMP_INTENSET_DOWN_Msk | LPCOMP_INTENSET_CROSS_Msk)) {
			nrf_drv_lpcomp_enable();
			nrf_delay_ms(10);
		}

		if (nrf52_softdevice_enabled())
			sd_power_system_off();
		else
			NRF_POWER->SYSTEMOFF = 1;

		// Use data synchronization barrier and a delay to ensure that no failure
		// indication occurs before System OFF is actually entered.
		__DSB();
		__NOP();
	}
}

void xs_sleep_get_reset_reason(xsMachine *the)
{
	xsmcSetInteger(xsResult, nrf52_get_reset_reason());	
}

void xs_sleep_get_latch(xsMachine *the)
{
	uint32_t pin = xsmcToInteger(xsArg(0));
	xsmcSetInteger(xsResult, nrf52_get_boot_latch(pin));
}

void xs_sleep_setup(xsMachine *the)
{
#ifdef mxDebug
	return;
#endif

	// We boot into System ON power mode.
	// Ensure all RAM is powered on and RAM retention in System OFF power mode is disabled.
	for (int i = 0; i <= 7; ++i)
		NRF_POWER->RAM[i].POWER = 0x03;
	NRF_POWER->RAM[8].POWER = 0x3F;

#if MODDEF_SLEEP_FASTRESET
	//Retain RAM2 Section 0 when in System OFF sleep to preserve fast reset setting
	NRF_POWER->RAM[2].POWERSET = 0x10000;
#endif

	// Restore or initialize system time
	if (kRamRetentionValueMagic == ((uint32_t *)MOD_TIME_RESTORE_MEM)[2]) {
		c_timeval tv = *((c_timeval *)MOD_TIME_RESTORE_MEM);
		modSetTime(tv.tv_sec);
	}
	else {
		modSetTimeZone(-8 * 3600);
		modSetDaylightSavingsOffset(3600);
		modSetTime(1601856000L);	// 10/5/2020 12:00:00 AM GMT
	}
	((uint32_t *)MOD_TIME_RESTORE_MEM)[2] = 0;
	
	// Power down ram to match requested amount.
	// Because this code is executed from the setup script, we know that the SoftDevice is disabled.
	int ramRequested = xsmcToInteger(xsArg(0));
	if (ramRequested <= 0)
		return;
		
	// All RAM between the RAM section below the stack and above the .NOINIT section is available.
	int ramAvailable = 0x20038000 - 0x20004200;
	
	int ramToPowerDown = ramAvailable - ramRequested;
	if (ramToPowerDown > 0) {
		// Start powering down from the 32 KB section (4) beneath the stack section
		uint8_t slave = 8, section = 4;
		uint8_t flags = 0;
		while (ramToPowerDown > 0) {
			if (8 == slave) {
				if (ramToPowerDown < 0x8000)
					break;
				ramToPowerDown -= 0x8000;
			}
			else {
				if (ramToPowerDown < 0x1000)
					break;
				ramToPowerDown -= 0x1000;
			}
			flags |= (1 << section);
			if (0 == section) {
				NRF_POWER->RAM[slave].POWERCLR = flags;
				flags = 0;
				section = 1;
				--slave;
			}
			else
				--section;
		}
		if (0 != flags)
			NRF_POWER->RAM[slave].POWERCLR = flags;
	}
}

void sleep_wake_on_timer()
{
	// Power off all ram sections except for retention memory (slave 2, section 0)
	NRF_POWER->RAM[0].POWERCLR = 0x03;
	NRF_POWER->RAM[1].POWERCLR = 0x03;
	NRF_POWER->RAM[2].POWERCLR = 0x02;
	NRF_POWER->RAM[3].POWERCLR = 0x03;
	NRF_POWER->RAM[4].POWERCLR = 0x03;
	NRF_POWER->RAM[5].POWERCLR = 0x03;
	NRF_POWER->RAM[6].POWERCLR = 0x03;
	NRF_POWER->RAM[7].POWERCLR = 0x03;
	NRF_POWER->RAM[8].POWERCLR = 0x3F;

	((uint32_t *)MOD_WAKEUP_REASON_MEM)[0] = 0;
	((uint32_t *)MOD_WAKEUP_REASON_MEM)[1] = 0;
	((uint32_t *)MOD_WAKEUP_REASON_MEM)[2] = 0;

	// Wait for RTC compare or configured digital/analog interrupt sources
	do {
		__WFE();
	} while (0 == (NVIC->ISPR[0] | NVIC->ISPR[1]));
    

	// Read RTC again and update saved time of day
	((c_timeval *)MOD_TIME_RESTORE_MEM)->tv_sec += ((portNRF_RTC_REG->COUNTER * 1000) >> 10) / 1000;
	((uint32_t *)MOD_TIME_RESTORE_MEM)[2] = kRamRetentionValueMagic;

    // If no RTC event pending, check for wake-up from configured digital/analog interrupt sources
    if (!nrf_rtc_event_pending(portNRF_RTC_REG, NRF_RTC_EVENT_COMPARE_0)) {
    	if ((0 != NRF_P0->LATCH) || (0 != NRF_P1->LATCH)) {
			((uint32_t *)MOD_WAKEUP_REASON_MEM)[0] = MOD_GPIO_WAKE_MAGIC;
			((uint32_t *)MOD_WAKEUP_REASON_MEM)[1] = NRF_P0->LATCH;
			((uint32_t *)MOD_WAKEUP_REASON_MEM)[2] = NRF_P1->LATCH;
			NRF_P0->LATCH = 0xFFFFFFFF;	// clear
			NRF_P1->LATCH = 0xFFFFFFFF;	// clear
    	}
    	else if (nrf_lpcomp_event_check(NRF_LPCOMP_EVENT_UP) && nrf_lpcomp_int_enable_check(LPCOMP_INTENSET_UP_Msk)) {
			((uint32_t *)MOD_WAKEUP_REASON_MEM)[0] = MOD_ANALOG_WAKE_MAGIC;
			NRF_LPCOMP->ENABLE = LPCOMP_ENABLE_ENABLE_Disabled << LPCOMP_ENABLE_ENABLE_Pos;
    	}
    	else if (nrf_lpcomp_event_check(NRF_LPCOMP_EVENT_DOWN) && nrf_lpcomp_int_enable_check(LPCOMP_INTENSET_DOWN_Msk)) {
			((uint32_t *)MOD_WAKEUP_REASON_MEM)[0] = MOD_ANALOG_WAKE_MAGIC;
			NRF_LPCOMP->ENABLE = LPCOMP_ENABLE_ENABLE_Disabled << LPCOMP_ENABLE_ENABLE_Pos;
    	}
    	else if (nrf_lpcomp_event_check(NRF_LPCOMP_EVENT_CROSS) && nrf_lpcomp_int_enable_check(LPCOMP_INTENSET_CROSS_Msk)) {
			((uint32_t *)MOD_WAKEUP_REASON_MEM)[0] = MOD_ANALOG_WAKE_MAGIC;
			NRF_LPCOMP->ENABLE = LPCOMP_ENABLE_ENABLE_Disabled << LPCOMP_ENABLE_ENABLE_Pos;
    	}
    }

	// Power on all RAM (seems to be required in order for reset to work)
	NRF_POWER->RAM[0].POWERSET = 0x03;
	NRF_POWER->RAM[1].POWERSET = 0x03;
	NRF_POWER->RAM[2].POWERSET = 0x02;
	NRF_POWER->RAM[3].POWERSET = 0x03;
	NRF_POWER->RAM[4].POWERSET = 0x03;
	NRF_POWER->RAM[5].POWERSET = 0x03;
	NRF_POWER->RAM[6].POWERSET = 0x03;
	NRF_POWER->RAM[7].POWERSET = 0x03;
	NRF_POWER->RAM[8].POWERSET = 0x3F;

	// Reset device
#if MODDEF_SLEEP_FASTRESET
	*((uint32_t*)DFU_DBL_RESET_MEM) = REBOOT_FAST_RESET;
#else 
	*((uint32_t*)DFU_DBL_RESET_MEM) = 0;
#endif
	NVIC_SystemReset();
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

static const uint32_t gPinsToDisable[] = {
#if defined MODDEF_SLEEP_SHUTDOWN_PINS
	MODDEF_SLEEP_SHUTDOWN_PINS
#endif
	0xffffffff
};

static void sleepPinsOff() {
	const uint32_t *pins = gPinsToDisable;
	while (*pins != 0xffffffff)
		nrf_gpio_cfg_default(*pins++);
}

