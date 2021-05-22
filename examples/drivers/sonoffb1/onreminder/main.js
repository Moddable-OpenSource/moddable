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
const brightness = 16;
const interval = 10 * 1000;		// ten seconds

light.write(brightness, 0, 0, 0, 0, 0); // warm

Timer.repeat(() => {
	light.write(0, 0, 0, 0, 0, brightness);	// blue
	Timer.delay(250);
	light.write(brightness, 0, 0, 0, 0, 0);	// warm
}, interval)
