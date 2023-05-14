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

	const conf = {
		filter: BME68x.FILTER_OFF,
		odr: BME68x.ODR_NONE,
		os_hum: BME68x.OS_16X,
		os_pres: BME68x.OS_1X,
		os_temp: BME68x.OS_2X
	};

	const heatr_conf = {
		enable: BME68x.ENABLE,
		heatr_temp: 300,
		heatr_dur: 100
	};

	bme.set_conf(conf);
	bme.set_heatr_conf(BME68x.FORCED_MODE, heatr_conf);

	bme.set_op_mode(BME68x.FORCED_MODE);
	Timer.repeat(() => {
		const data = bme.get_data(BME68x.FORCED_MODE);
		if (data)
			trace(JSON.stringify(data[0], undefined, 3), "\n");

		bme.set_op_mode(BME68x.FORCED_MODE);
	}, bme.get_meas_dur(BME68x.FORCED_MODE, conf) + heatr_conf.heatr_dur + 0.5);
}
