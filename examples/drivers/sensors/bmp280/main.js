/*
 * Copyright (c) 2021-2023 Moddable Tech, Inc.
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

import BMP280 from "embedded:sensor/Barometer-Temperature/BMP280";
import Timer from "timer";

const sensor = new BMP280({ sensor: device.I2C.default });
		
function CtoF(c) { return (c*1.8)+32; }
function PatoInHg(Pa) { return Pa * 0.0002953; }

Timer.repeat(() => {
	const sample = sensor.sample();

    trace(`Temperature: ${sample.thermometer.temperature?.toFixed(2)} C -- ${CtoF(sample.thermometer.temperature)?.toFixed(2)} F\n`);
    trace(`Pressure: ${sample.barometer.pressure?.toFixed(2)} Pa -- ${PatoInHg(sample.barometer.pressure)?.toFixed(3)} inHg\n`);
}, 2000);

