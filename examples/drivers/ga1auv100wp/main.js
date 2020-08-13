/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import GA1AUV100WP from "ga1auv100wp";
import Timer from "timer";

let sensor = new GA1AUV100WP();
sensor.configure({operation: "als"});

Timer.repeat(() => {
	let value = sensor.sample();
	if ("lux" in value)
		 trace(`LUX ${value.lux}\n`);
	 if ("uv" in value)
		 trace(`UV ${value.uv}\n`);
}, 100);
