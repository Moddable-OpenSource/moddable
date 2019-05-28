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

// Sonoff B1 write order: W C 0 G R B.

let light = new MY92x1;
const brightness = 16;
let interval = 750;
let step = 0;

Timer.repeat(id => {
	switch (step) {
		case 0:		// red
			light.write(0, 0, 0, brightness, 0, 0);
			break;
		case 1:		// green
			light.write(0, 0, 0, 0, brightness, 0);
			break;
		case 2:		// blue
			light.write(0, 0, 0, 0, 0, brightness);
			break;
		case 3:		// warm
			light.write(brightness, 0, 0, 0, 0, 0);
			break;
		case 4:		// cool
			light.write(0, brightness, 0, 0, 0, 0);
			break;
		case 5:		// cool + warm
			light.write(brightness, brightness, 0, 0, 0, 0);
			break;
	}

	step += 1;
	if (step > 5) {
		step = 0;

		interval = Math.max(100, interval - 125);
		Timer.schedule(id, interval, interval);
	}
}, 750);
