/*
 * Copyright (c) 2023-2024  Moddable Tech, Inc.
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

import Sensor from "embedded:sensor/Barometer-Humidity-Temperature/BME68x";
import {} from "piu/MC";

const BLUE = "#192eab";
const WHITE = "white";

const style = Object.freeze({ font:"semibold 40px Open Sans", color:[WHITE,BLUE] }, true);

class PlainBehavior extends Behavior {
	onCreate(application) {
		try {
			this.sensor = new Sensor({ sensor: device.I2C.default });
		}
		catch(e) {
			this.sensor = {
				sample() {
 					return {
					   thermometer: {
						  temperature: 33.68404006958008
					   },
					   hygrometer: {
						  humidity: 0.268742733001709
					   },
					   barometer: {
						  pressure: 100989.90625
					   }
				 	}
				}
			}
		}
		this.button = new device.peripheral.button.Flash({
			onPush: () => {
				const value = this.button.pressed;
				application.state = value;;
				let label = application.first.first;
				label.state = value;
				label = label.next;
				label.state = value;
				label = label.next;
				label.state = value;
			}
		});
	}
	onDisplaying(application) {
		application.interval = 250;
		application.start();
	}
	onTimeChanged(application) {
		const sample = this.sensor.sample();
		if (sample) {
			let label = application.first.first;
			label.string = Math.round(sample.thermometer.temperature) + "°C";
			label = label.next;
			label.string = Math.round(sample.hygrometer.humidity * 100) + "% RH";
			label = label.next;
			label.string = Math.round(sample.barometer.pressure / 100) + " hPa";
		}
	}
};

let PlainApplication = Application.template($ => ({
	skin:{ fill:[BLUE,WHITE] }, 
	Behavior:PlainBehavior,
	contents: [
		Column($, {
			top: 30,
			contents: [
				Label($, { style } ),
				Label($, { style } ),
				Label($, { style } ),
			]
		})
	]
}));

export default function () {
	return new PlainApplication(null, { displayListLength:4096, touchCount:0 });
}

