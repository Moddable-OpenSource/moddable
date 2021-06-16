/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

import device from "embedded:provider/builtin";
import Sensor from "embedded:sensor/AirQuality/CCS811";
import Humidity from "embedded:sensor/Humidity-Temperature/HTU21D";
import Timer from "timer";

const sensor = new Sensor({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});
const humidity = new Humidity({ sensor: device.I2C.default });

while (!sensor.available)
	Timer.delay(500);

function CtoF(c) { return (c*1.8)+32; }

Timer.repeat(() => {
	const h = humidity.sample();
	trace(`humidity: ${h.humidity.toFixed(2)} %RH, temperature: ${CtoF(h.temperature).toFixed(2)} F\n`);
	sensor.configure({
		environmentalData: {
			humidity: h.humidity,
			temperature: h.temperature
		}
	});

	const sample = sensor.sample();
	trace(`eCO2: ${sample.eCO2} ppm, VOC: ${sample.TVOC} ppb\n`);

}, 10000);

