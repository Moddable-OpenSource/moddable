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

import Sensor from "embedded:sensor/CarbonDioxideGasSensor-VOCSensor/CCS811";
import Humidity from "embedded:sensor/Humidity-Temperature/SI7020";
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
	trace(`humidity: ${h.hygrometer.humidity?.toFixed(2)} %RH, temperature: ${CtoF(h.thermometer.temperature)?.toFixed(2)} F\n`);
	sensor.configure({
		humidity: h.hygrometer.humidity,
		temperature: h.thermometer.temperature
	});

	const sample = sensor.sample();
	trace(`eCO2: ${sample.carbonDioxideGasSensor.CO2} ppm, VOC: ${sample.vocSensor.tvoc} ppb\n`);

}, 10000);

