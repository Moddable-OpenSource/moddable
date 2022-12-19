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

import device from "embedded:provider/builtin";
import Moisture from "embedded:sensor/SoilMoistureSensor/Capacitive";
import Timer from "timer";

const sensor = new Moisture({
	sensor: device.Analog.default
});

sensor.configure({
	averaging: 8,
});

Timer.repeat(id => {
	trace(`${sensor.sample().moisture.toFixed(2)}\n`);
}, 2000);
