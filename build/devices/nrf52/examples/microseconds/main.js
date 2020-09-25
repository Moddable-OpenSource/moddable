/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import Microseconds from "microseconds";
import Timer from "timer";

// timing long operation
let sum = 0;
Microseconds.start();
for (let i = 0; i < 100; ++i) {
	sum += i;
	for (let j = 0; j < 100; ++j)
		sum += j;
}
trace(`operation microseconds: ${Microseconds.stop()}\n`);

// timing ms delay
Microseconds.start();
Timer.delay(2000);
trace(`delay microseconds: ${Microseconds.stop()}\n`);

// timing one second elapsed repeats
let count = 1;
Microseconds.start();
Timer.repeat(id => {
	trace(`repeat ${count}, elapsed microseconds: ${Microseconds.get()}\n`);
	if (6 == ++count) {
		Microseconds.stop();
		Timer.clear(id);
	}
}, 1000);
