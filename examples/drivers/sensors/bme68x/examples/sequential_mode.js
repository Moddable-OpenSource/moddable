/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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

import {BME68x} from "embedded:sensor/Barometer-Humidity-Temperature/BME68x"; 
import Timer from "timer";

export default function() {
	const bme = new BME68x({
		sensor: device.I2C.default
	});

	const heatr_conf = {
		enable: BME68x.ENABLE,
		heatr_temp_prof: [200, 240, 280, 320, 360, 360, 320, 280, 240, 200],
		heatr_dur_prof: [100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
	};

	const conf = bme.get_conf();
	conf.filter = BME68x.FILTER_OFF;
	conf.odr = BME68x.ODR_NONE; /* This parameter defines the sleep duration after each profile */
	conf.os_hum = BME68x.OS_16X;
	conf.os_pres = BME68x.OS_1X;
	conf.os_temp = BME68x.OS_2X;

	bme.set_conf(conf);
	bme.set_heatr_conf(BME68x.SEQUENTIAL_MODE, heatr_conf);

	bme.set_op_mode(BME68x.SEQUENTIAL_MODE);
	Timer.repeat(() => {
		const data = bme.get_data(BME68x.SEQUENTIAL_MODE);
		data?.forEach(d => trace(JSON.stringify(d, undefined, 3), "\n"));
	}, bme.get_meas_dur(BME68x.SEQUENTIAL_MODE, conf) + heatr_conf.heatr_dur_prof[0] + 0.5);
}
