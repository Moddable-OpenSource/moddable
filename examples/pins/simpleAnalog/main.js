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

import Analog from "pins/analog";
import Timer from "timer";

Timer.repeat(id => {
	let value1 = Analog.read(26);
	trace(`value1: ${value1}\n`);
//	let value2 = Analog.read(2);
//	trace("value1: " + value1 + " value2: " + value2 + "\n");
}, 250);
