/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

class Sleep {
	static getRetainedValue(index) @ "xs_sleep_get_retained_value";
	static setRetainedValue(index, value) @ "xs_sleep_set_retained_value";
	
	static set powerMode(it) @ "xs_sleep_set_power_mode";

	static deep(ms) @ "xs_sleep_deep";
	
	static get resetReason() @ "xs_sleep_get_reset_reason";
	static getLatch(pin) @ "xs_sleep_get_latch";
	
	static setup() @ "xs_sleep_setup";
};

const PowerMode = {
	ConstantLatency: 1,
	LowPower: 2
};
Object.freeze(PowerMode);

const SleepMode = {
	SystemOn: 1,
	SystemOff: 2
};
Object.freeze(SleepMode);

const ResetReason = {
	RESETPIN: 1 << 0,	// reset pin
	DOG: 1 << 1,		// watchdog
	SREQ: 1 << 2,		// software reset
	LOCKUP: 1 << 3,		// cpu lockup
	GPIO: 1 << 16,		// detected by the use of DETECT signal from GPIO (wake on digital)
	LPCOMP: 1 << 17,	// detected by the use of ANDETECT signal from GPIO (wake on analog)
	DIF: 1 << 18,		// debugger interface
	NFC: 1 << 19		// nfc field detect
};
Object.freeze(ResetReason);

export {Sleep as default, Sleep, PowerMode, SleepMode, ResetReason};
