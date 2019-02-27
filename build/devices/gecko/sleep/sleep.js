/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
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

export default Sleep;

Object.freeze(Sleep.prototype);
