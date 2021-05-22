/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 * Copyright (c) Wilberforce
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
 
let count = 0;

import Timer from "timer";

Timer.delay(1);
lights.fill(lights.makeRGB(255, 255, 255)); lights.update();
Timer.delay(5000);
lights.fill(lights.makeRGB(255, 0, 0)); lights.update();
Timer.delay(5000);
lights.fill(lights.makeRGB(0, 255, 0)); lights.update();
Timer.delay(5000);
lights.fill(lights.makeRGB(0, 0, 255)); lights.update();
Timer.delay(5000);

let value = 0x01;
Timer.repeat(() => {
	let v = value;
	for (let i = 0; i < lights.length; i++) {
		v <<= 1;
		if (v == (1 << 24))
			v = 1;
		lights.setPixel(i, v);
	}

	lights.update();

	value <<= 1;
	if (value == (1 <<24))
		value = 1;
}, 33);

button.a.onChanged = function() {
	if (button.a.read()) {
		return;
	}
	count++;
	if (count >= 16) {
		count = 7;
	}
	trace(count,'\n');
	lights.fill(lights.makeRGB(255, 255, 255)); lights.update();
}
