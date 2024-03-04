/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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

// immediate (setImmediate)
Timer.set(() => trace("immediate\n"));

// one shot (setTimeout)
Timer.set(() => trace("oneshot\n"), 600);

// repeat (setInterval)
let count = 0;
Timer.repeat(id => {
	trace(`repeat ${count} \n`);
	if (5 == ++count)
		Timer.clear(id);
}, 250);

// reschedule
let counter = 0;
Timer.set(id => {
	++counter;
	if (1 === counter) {
		trace(`rescheduled - first time\n`);
		Timer.schedule(id, 500, 250);	// reschedule for 500ms and then repeating for 250 ms
	}
	else {
		if (counter < 6)
			trace(`rescheduled - ${counter}\n`);
		else {
			Timer.clear(id);
			trace("rescheduled - clear\n")
		}
	}
}, 3000);

// unschedule
Timer.set(id => {
	trace("unschedule\n");
	Timer.schedule(id);
}, 2500);
