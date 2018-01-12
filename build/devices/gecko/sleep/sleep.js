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
	constructor(dictionary) { global.sleepers = []; }
	static getPersistentValue(reg) @ "xs_get_persistent_value";
	static setPersistentValue(reg, value) @ "xs_set_persistent_value";
	static sleepEM4(ms) {
		sleepers.forEach(callback => (callback)());
		this.doSleepEM4(ms);
	}
	static doSleepEM4(ms) @ "xs_sleep_enter_em4";
	static getWakeupCause() @ "xs_sleep_get_reset_cause";
	static getWakeupPin() @ "xs_sleep_get_wakeup_pin";
};

Sleep.ExternalReset		= 0b00000001;
Sleep.SysRequestReset	= 0b00000010;
Sleep.EM4WakeupReset	= 0b00000100;

Object.freeze(Sleep.prototype);

export default Sleep;

