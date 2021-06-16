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
import { BMP180, Config } from "embedded:sensor/AtmosphericPressure-Temperature/BMP180";
import Timer from "timer";

const sensor = new BMP180({ sensor: device.I2C.default });

sensor.configure({
	mode: Config.Mode.ULTRAHIGHRES
});

function CtoF(c) { return (c*1.8)+32; }
function PatoInHg(Pa) { return Pa * 0.0002953; }

Timer.repeat(() => {
	const sample = sensor.sample();

    trace(`Temperature: ${sample.temperature.toFixed(2)} C -- ${CtoF(sample.temperature).toFixed(2)} F\n`);
    trace(`Pressure: ${sample.pressure.toFixed(2)} Pa -- ${PatoInHg(sample.pressure).toFixed(3)} inHg\n`);

}, 2000);

