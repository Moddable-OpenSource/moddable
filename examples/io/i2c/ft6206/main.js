/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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

import Touch from "embedded:sensor/touch/FT6x06";

const touch = new Touch({
			sensor: {
				...device.I2C.default,
				io: device.io.SMBus
			} });

touch.configure({
	flip: "hv"
});

System.setInterval(() => {
	const points = touch.sample();

	points?.forEach((point, i) => {
		const id = point.id;
		delete point.id;
		trace(`Point ${id}: ${JSON.stringify(point)}\n`);
	});
}, 33);
