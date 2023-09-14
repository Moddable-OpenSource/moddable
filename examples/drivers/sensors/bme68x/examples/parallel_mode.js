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
		heatr_temp_prof: [320, 100, 100, 100, 200, 200, 200, 320, 320, 320],
		heatr_dur_prof: [5, 2, 10, 30, 5, 5, 5, 5, 5, 5]
	};

	const conf = bme.get_conf();
	conf.filter = BME68x.FILTER_OFF;
	conf.odr = BME68x.ODR_NONE;
	conf.os_hum = BME68x.OS_1X;
	conf.os_pres = BME68x.OS_16X;
	conf.os_temp = BME68x.OS_2X;

	bme.set_conf(conf);
	heatr_conf.shared_heatr_dur = 140 - bme.get_meas_dur(BME68x.PARALLEL_MODE, conf);
	bme.set_heatr_conf(BME68x.PARALLEL_MODE, heatr_conf);

	bme.set_op_mode(BME68x.PARALLEL_MODE);
	Timer.repeat(() => {
		const data = bme.get_data(BME68x.PARALLEL_MODE);
		data?.forEach(d => {
			if (d.status & BME68x.VALID_DATA)
				trace(JSON.stringify(d, undefined, 3), "\n")
		});
	}, bme.get_meas_dur(BME68x.PARALLEL_MODE, conf) + heatr_conf.shared_heatr_dur + 0.5);
}
