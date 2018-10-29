/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import Timer from "timer";
import PWM from "pins/pwm";
import config from "mc/config";

let r = new PWM({ pin: config.r_pin });
let g = new PWM({ pin: config.g_pin });
let b = new PWM({ pin: config.b_pin });

r.write(1023);
b.write(1);
g.write(1023);

let rVal = 1023;
let gVal = 1023;
let direction = 0;
Timer.repeat(() => {
	switch (direction) {
		case 0:
			rVal -= 20;
			if (rVal <= 1) {
				rVal = 1;
				direction++;
			}
			r.write(rVal);
			break;
		case 1:
			rVal += 20;
			if (rVal >= 1023) {
				rVal = 1023;
				direction++;
			}
			r.write(rVal);
			break;
		case 2:
			gVal -= 20;
			if (gVal <= 1) {
				gVal = 1;
				direction++;
			}
			g.write(gVal);
			break;
		case 3:
			gVal += 20;
			if (gVal >= 1023) {
				gVal = 1023;
				direction = 0;
			}
			g.write(gVal);
			break;
	}
}, 50);
