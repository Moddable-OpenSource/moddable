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
import {GP2AP01VT00F, Emitter, Interval} from "gp2ap01vt00f";

let sensor = new GP2AP01VT00F();
sensor.configure({emitter: Emitter.mA_59, interval: Interval.ms_004})

Timer.repeat(() => {
	let sample = sensor.sample();
	trace(`${sample.distance}mm\n`);
}, 100);
