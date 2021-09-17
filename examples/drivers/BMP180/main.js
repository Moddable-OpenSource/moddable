/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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
import BMP180 from "bmp180";

const BMP180_ADDR = 0x77

let sensor = new BMP180({address: BMP180_ADDR});

Timer.repeat(() => {
	let measure = sensor.sample();
	trace(measure.temperature, '° ', measure.pressure, 'mb', '\n');
}, 250);


