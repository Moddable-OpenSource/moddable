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
import Timer from "timer";

const sensor = new Sensor({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

sensor.configure({ mode: 2 });

trace(`waiting for data\n`);
while (!sensor.available)
	Timer.delay(500);

Timer.repeat(() => {
	if (sensor.available) {
		const sample = sensor.sample();

		trace(`eCO2: ${sample.carbonDioxideGasSensor.CO2} ppm, VOC: ${sample.vocSensor.tvoc} ppb\n`);
	}
	else
		trace(`no data\n`);
}, 5000);

