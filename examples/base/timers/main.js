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

// immediate (setImmediate)
Timer.set(immediate);

// one shot (setTimeout)
Timer.set(oneshot, 600);

// repeat (setInterval)
let count = 0;
Timer.repeat(repeat, 250);

function immediate(id)
{
	trace("immediate\n");
}

function oneshot(id)
{
	trace("oneshot\n");
}

function repeat(id)
{
	trace(`repeat ${count} \n`);
	if (5 == ++count)
		Timer.clear(id);
}
