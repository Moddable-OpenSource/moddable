/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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

import Servo from "pins/servo";
import Timer from "timer";

const servoPin = undefined;		// define pin number here
if (undefined === servoPin)
	throw new Error("servoPin not configured");

const servo = new Servo({pin: servoPin});

let angle = 0;
Timer.repeat(() => {
	angle += 2.5;
	if (angle > 180) angle -= 180;
	servo.write(angle);
}, 250);
