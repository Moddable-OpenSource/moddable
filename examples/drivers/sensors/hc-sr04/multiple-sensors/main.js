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
import Timer from "timer";

const triggers = [];
let onTrigger = 0;

function toggleOutput() {
	triggers[onTrigger].output.write(1);
	triggers[onTrigger].output.write(0);
}	

for (let i = 0; i < config.triggerPins.length; i++) {
	const pin = config.triggerPins[i];
	const output = new device.io.Digital({
		mode: device.io.Digital.Output,
		pin
	});
	output.write(0);
	triggers.push({
		pin,
		output
	});
}

let sensor = new SR04({	
	sensor: {
		pin: config.echoPin,
		io: device.io.PulseWidth
	},
	onAlert: () => {
		const value = sensor.sample();

		if (value !== undefined) {
			if (value.near) {
				trace(`Trigger ${triggers[onTrigger].pin} Distance: ${value.distance} cm\n`);
			} else {
				trace(`Trigger ${triggers[onTrigger].pin} Out of range (beyond ${value.max} cm)\n`);
			}
		}

		onTrigger++;
		onTrigger %= triggers.length;
		Timer.delay(150);
		toggleOutput();
	}
});

sensor.configure({suppressDuplicates: false});
toggleOutput();
