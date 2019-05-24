/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

import MY92x1 from "my92x1";
import Timer from "timer";

const light = new MY92x1;
const onBrightness = 128;		// 50 %
const onTime = 10 * 1000;		// ten seconds
const fadeDuration = 2 * 1000;	// two seconds
const fadeStep = 10;
const fadedBrightness = 12;		// 10%

light.write(0, onBrightness, 0, 0, 0, 0); // cool

Timer.set(() => {
	let brightness = onBrightness;
	let step = (onBrightness - fadedBrightness) / (fadeDuration / fadeStep);
	Timer.repeat(id => {
		brightness -= step;
		if (brightness <= fadedBrightness) {
			brightness = fadedBrightness;
			Timer.clear(id);
		}
		light.write(0, brightness, 0, 0, 0, 0);
	}, fadeStep);
}, onTime);
