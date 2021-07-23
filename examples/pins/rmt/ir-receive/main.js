/*
 * Copyright (c) 2021 Satoshi Tanaka
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import RMT from "pins/rmt";
import Timer from "timer";

const IrReceivePin = 33;
let inputRMT = new RMT({pin: IrReceivePin, channel: 0, divider: 80, direction: "rx", filter: 255, timeout: 30000, ringbufferSize: 512});

const data = new Uint16Array(512);

Timer.repeat(() => {
	let result = inputRMT.read(data.buffer);
	if (result.count) {
		trace(`rawData[${result.count}] =\n[`);
		for (let i = 0; i < result.count; i++) {
			trace(`${data[i]}`);
			if(i != result.count -1) trace(", ");
		}
		trace(`]\n`);
	}
}, 100);

