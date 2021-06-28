/*
 * Copyright (c) 2021 Moddable Tech, Inc.
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

import RMT from "pins/rmt";
import Timer from "timer";

let inputRMT = new RMT({pin: 17, channel: 3, divider: 100, direction: "rx", filter: 100, timeout: 7000, ringbufferSize: 512});

const data = new Uint16Array(512);

Timer.repeat(() => {
	let result = inputRMT.read(data.buffer);
	if (result.count) {
		let value = result.phase;
		for (let i = 0; i < result.count; i++) {
			trace(`Pulse of value ${value} for duration ${data[i]}\n`);
			value ^= 1;
		}
	}
}, 20);

