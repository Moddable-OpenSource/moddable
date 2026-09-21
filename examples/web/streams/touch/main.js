/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import Touch from "embedded:sensor/Touch/FT6x06";
import { SensorStreamMixin } from "sensorstream";

const TouchStream = SensorStreamMixin(Touch);

async function test() {
	const stream = new TouchStream({
		sensor: {
			...device.i2c.default,
			io: device.io.SMBus
		},
		interval: 33
	});
	for await (let point of stream) {
		trace(`point: ${point.id} x: ${point.x} y: ${point.y}\n`);
	}
}
test();
