/*
 * Copyright (c) 2022-2023 Moddable Tech, Inc.
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

import SR04 from "embedded:sensor/Proximity/HC-SR04";
import config from "mc/config";

let sensor = new SR04({	
	trigger: {
		pin: config.triggerPin,
		io: device.io.Digital,
	},
	sensor: {
		pin: config.echoPin,
		io: device.io.PulseWidth
	},
	onAlert: () => {
		const value = sensor.sample();

		if (value !== undefined) {
			if (value.near) {
				trace(`Distance: ${value.distance} cm\n`);
			} else {
				trace(`Out of range (beyond ${value.max} cm)\n`);
			}
		}
	}
});
