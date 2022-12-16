/*
 * Copyright (c) 2022 Moddable Tech, Inc.
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

let sensor = new SR04({	
	output: 13,
	input: 12,
	inputIO: device.io.PulseWidth,
	outputIO: device.io.Digital,
	onAlert: value => {
		trace(`Distance: ${value} cm\n`);
	}
});
