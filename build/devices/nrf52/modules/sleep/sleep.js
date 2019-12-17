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

class Sleep {
	static install(handler) {
		// this dance allows handlers to be installed both at preload and run time
		if (Object.isFrozen(Sleep.prototype.handlers))
			Sleep.prototype.handlers = Array.from(Sleep.prototype.handlers);
		Sleep.prototype.handlers.push(handler);
	}

	static clearRetainedBuffer() @ "xs_sleep_clear_retained_buffer";
	static getRetainedBuffer() @ "xs_sleep_get_retained_buffer";
	static setRetainedBuffer() @ "xs_sleep_set_retained_buffer";

	static set powerMode() @ "xs_sleep_set_power_mode";

	static deep() {
		Sleep.prototype.handlers.forEach(handler => (handler)());
		Sleep.#deep();
	}
	
	static #deep() @ "xs_sleep_deep";	// System OFF sleep mode

	static get resetReason() @ "xs_sleep_get_reset_reason";
	static get resetPin() @ "xs_sleep_get_reset_pin";
	
	static wakeOnDigital(pin) @ "xs_sleep_wake_on_digital"
	
	static wakeOnAnalog(channel, configuration) {
		switch(configuration.mode) {
			case AnalogDetectMode.Crossing:
			case AnalogDetectMode.Up:
			case AnalogDetectMode.Down:
				break;
			default:
				throw new Error("invalid analog detect mode");
				break;
		}
		Sleep.#wakeOnAnalog(channel, configuration.mode, configuration.value);
	}
	
	static wakeOnInterrupt(pin) @ "xs_sleep_wake_on_interrupt"
	
	static #wakeOnAnalog() @ "xs_sleep_wake_on_analog"
};
Sleep.prototype.handlers = [];

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

const AnalogDetectMode = {
	Crossing: 0,
	Up: 1,
	Down: 2
};
Object.freeze(AnalogDetectMode);

const ResetReason = {
	RESETPIN: 1 << 0,
	DOG: 1 << 1,
	SREQ: 1 << 2,
	LOCKUP: 1 << 3,
	GPIO: 1 << 16,		// detected by the use of DETECT signal from GPIO (wake on digital)
	LPCOMP: 1 << 17,	// detected by the use of ANDETECT signal from GPIO (wake on analog)
	DIF: 1 << 18,
	NFC: 1 << 19
};
Object.freeze(ResetReason);

export {Sleep as default, Sleep, PowerMode, SleepMode, AnalogDetectMode, ResetReason};
