/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Digital from "pins/digital";
import {Sleep, ResetReason} from "sleep";
import config from "mc/config";

let wakenWith = "?";

export default class {
	constructor() {
	}
	getRetainedValue(index) {
		return Sleep.getRetainedValue(index);
	}
	setRetainedValue(index, value) {
		Sleep.setRetainedValue(index, value);
	}
	sleep(options) {
		if (options.accelerometer) {
			let digital = new Digital({
				pin: config.lis3dh_int1_pin,
				mode: Digital.Input,		//  mode: Digital.InputPullUp,
				wakeEdge: Digital.WakeOnFall,
				onWake() {}
			});
		}
		if (options.button) {
			let digital = new Digital({
				pin: config.button1_pin,
				mode: Digital.InputPullUp,
				wakeEdge: Digital.WakeOnFall,
				onWake() {}
			});
		}
		if (options.jogdial) {
			let digital2 = new Digital({
				pin: config.jogdial.button,
				mode: Digital.InputPullUp,
				wakeEdge: Digital.WakeOnFall,
				onWake() {}
			});
		}
		if (options.duration)
			Sleep.deep(options.duration);
		else
			Sleep.deep();
	}
	get wakenWith() {
		return wakenWith;
	}
	static setup() {
// 		wakenWith = Sleep.resetReason;
		if (Sleep.resetReason == ResetReason.RESETPIN)
			wakenWith = "reset";
		else if (Sleep.getLatch(config.lis3dh_int1_pin))
			wakenWith = "accelerometer";
		else if (Sleep.getLatch(config.button1_pin))
			wakenWith = "button";
		else if (Sleep.getLatch(config.jogdial.button))
			wakenWith = "jogdial";
		else
			wakenWith = "timer";

	}
}
