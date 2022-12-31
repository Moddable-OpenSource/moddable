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


import Timer from "timer";
import APDS9301 from "embedded:sensor/AmbientLight/APDS9301";
import VL6180 from "embedded:sensor/AmbientLight-Proximity/VL6180";


let vl6180 = new VL6180({
    sensor: {
        ...device.I2C.default,
        io: device.io.I2C	
    }
});

let apds9301 = new APDS9301({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

trace(`APDS9301 configuration is: ${JSON.stringify(apds9301.configuration)}\n`);
trace(`APDS9301 identification is: ${JSON.stringify(apds9301.identification)}\n\n`);


trace(`VL6180 configuration is: ${JSON.stringify(vl6180.configuration)}\n`);
trace(`VL6180 identification is: ${JSON.stringify(vl6180.identification)}\n`);

Timer.repeat(() => {
	const aSample = apds9301.sample();
    const vSample = vl6180.sample();

	trace(`APDS9301: ${aSample.illuminance}, VL6180: ${vSample.lightmeter.illuminance}\n`);
}, 1000);
