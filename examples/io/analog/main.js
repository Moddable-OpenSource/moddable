/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
import config from "mc/config";
import Analog from "embedded:io/analog";

const potConfig = {
	io: Analog,
	pin: 31
};

const sensor = new Analog(potConfig);

Timer.repeat(id => {
	let sample = sensor.read();
	trace(`read: ${sample}\n`);
}, 100);
