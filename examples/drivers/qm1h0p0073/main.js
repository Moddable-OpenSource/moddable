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
import QM1H0P0073 from "qm1h0p0073";

let sensor = new QM1H0P0073();
Timer.repeat(() => {
	let sample = sensor.sample();
	trace(`temperature: ${sample.temperature}, humidity: ${sample.humidity}\n`);
}, 100);
