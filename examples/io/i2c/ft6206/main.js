/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

import Touch from "sensor/touch";

const touch = new Touch(device.I2C.default);

while (true) {
	const points = touch.sample();
	if (!points.length)
		continue;

	if (points[0])
		trace(`Point 1 {${points[0].x}, ${points[0].y}}\n`);

	if (points[1])
		trace(`Point 2 {${points[1].x}, ${points[1].y}}\n`);
}
