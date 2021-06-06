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
import NeoPixel from "neopixel";
import Timer from "timer";

const np = new NeoPixel({length: 1, pin: 1, order: "GRB"});

Timer.delay(1);
np.fill(np.makeRGB(255, 255, 255)); np.update();
Timer.delay(500);
np.fill(np.makeRGB(255, 0, 0)); np.update();
Timer.delay(500);
np.fill(np.makeRGB(0, 255, 0)); np.update();
Timer.delay(500);
np.fill(np.makeRGB(0, 0, 255)); np.update();
Timer.delay(500);

let value = 0x01;
Timer.repeat(() => {
	let v = value;
	for (let i = 0; i < np.length; i++) {
		v <<= 1;
		if (v == (1 << 24))
			v = 1;
		np.setPixel(i, v);
	}

	np.update();

	value <<= 1;
	if (value == (1 <<24))
		value = 1;
}, 33);
