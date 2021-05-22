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

if (!globalThis.lights || !globalThis.accelerometer || !globalThis.button)
	throw new Error("this M5 example requires lights, accelerometer, and a button");

let random = false;


let brightness = 126;
lights.brightness = brightness;

let buttonPressed = false;

accelerometer.onreading = function(values) {
	if (random) {
		// random colours
		for (let i = 0, length = lights.length; i < length; i++)
			lights.setPixel(i, lights.makeRGB(255 * Math.random(), 255 * Math.random(), 255 * Math.random()));
	}
	else {
		// Change colour of lights depending on orientation
		const x = Math.min(Math.max(values.x, -1), 1) / 2;
		const y = Math.min(Math.max(values.y, -1), 1) / 2;
		const z = Math.min(Math.max(values.z, -1), 1) / 2;
		lights.fill(lights.makeRGB((128 + x * 255) | 0, (128 + y * 255) | 0, (128 + z * 255) | 0));
	}

	if (buttonPressed) {
		// adjust brightness
		brightness += 5;
		if (brightness >= 255)
			brightness = 1;
		lights.brightness = brightness;
	}

	lights.update();
}

accelerometer.start(50);

// double click of button toggles random lights
let last = 0;
button.a.onChanged = function() {
	buttonPressed = !button.a.read();
	if (!buttonPressed)
		return;

	const now = Date.now();
	if ((now - last) < 500)
		random = !random;

	last = now;
}
