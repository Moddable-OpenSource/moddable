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
import LIS3DH from "lis3dh";
import {DataRate} from "lis3dh";

let sensor = new LIS3DH({});
sensor.configure({rate: DataRate.DATARATE_10_HZ});

Timer.repeat(() => {
	let values = sensor.sample();
	trace(`x: ${values.x}, y: ${values.y}, z: ${values.z}\n`);
}, 100);
