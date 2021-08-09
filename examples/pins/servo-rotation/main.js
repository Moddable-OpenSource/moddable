/*
 * Copyright (c) 2021  Satoshi Tanaka
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

let servo = new Servo({pin: 26});
let increase = 1;
let pulseWidth = 1500;

Timer.repeat(() => {
	if(pulseWidth > 2000 || pulseWidth < 1000) increase *= -1;
	pulseWidth += 50 * increase;
	servo.writeMicroseconds(pulseWidth);
}, 200);
