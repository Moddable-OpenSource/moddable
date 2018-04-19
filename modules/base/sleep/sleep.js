/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

export class Sleep {
	static install(handler) {
		// this dance allows handlers to be installed both at preload and run time
		if (Object.isFrozen(Sleep.prototype.handlers))
			Sleep.prototype.handlers = Array.from(Sleep.prototype.handlers);
		Sleep.prototype.handlers.push(handler);
	}

	static getPersistentValue(index) @ "xs_get_persistent_value";
	static setPersistentValue(index, value) @ "xs_set_persistent_value";
	static deep(ms) {
		Sleep.prototype.handlers.forEach(handler => (handler)());
		this.doSleepEM4(ms);
	}
	static doSleepEM4(ms) @ "xs_sleep_enter_em4";
	static getWakeupCause() @ "xs_sleep_get_reset_cause";
	static getWakeupPin() @ "xs_sleep_get_wakeup_pin";

	static getIdleSleepLevel() @ "xs_sleep_get_idle_sleep_level";
	static setIdleSleepLevel(level) @ "xs_sleep_set_idle_sleep_level";
};

Sleep.PowerOnReset		= 0b00000001;			// power applied
Sleep.ExternalReset		= 0b00000010;			// reset button
Sleep.SysRequestReset	= 0b00000100;			// software reset
Sleep.EM4WakeupReset	= 0b00001000;			// woke from EM4
Sleep.BrownOutReset		= 0b00010000;			// power dipped too low
Sleep.LockupReset		= 0b00100000;			// device hung
Sleep.WatchdogReset		= 0b01000000;			// watchdog timer expired

/* Do not call Object.freeze on Sleep.prototype */

export default Sleep;
