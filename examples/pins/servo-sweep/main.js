/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

/* let servo = new Servo({pin: 5}); */
let servo = new Servo({pin: 5, min: 500, max: 2400});

let angle = 0;
Timer.repeat(() => {
	servo.write(angle);
	angle = angle ? 0 : 180;
}, 3000);
