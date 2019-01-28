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
import Digital from "pins/digital";

Digital.configure(2, 0);
Digital.configure(3, 1);

let count = 0;
Timer.repeat(() => {
	trace(`repeat ${count} \n`);
	if (0 == ++count % 3) {
		Digital.write(2, 1);
		Digital.write(3, 0);
	}
	else if (1 == count % 3) {
		Digital.write(2, 0);
		Digital.write(3, 1);
	}
	else {
		Digital.write(2, 0);
		Digital.write(3, 0);
	}
}, 250);
