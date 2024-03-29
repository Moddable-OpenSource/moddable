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

import Instrumentation from "instrumentation";

import Timer from "timer"

Timer.set(() => {}, 5000)


const stackIndex = Instrumentation.map("XS Stack Used");
trace(`XS Stack Used: ${Instrumentation.get(stackIndex)}\n`);

for (let i = 1; true; i++) {
	const name = Instrumentation.name(i);
	if (!name)
		break;
	const value = Instrumentation.get(i);
	trace(`${name}: ${value}\n`);
}
