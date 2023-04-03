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

import Timer from "timer";
import PZEM004T from "embedded:sensor/Energy/PZEM004T";
import config from "mc/config";

let sensor = new PZEM004T({
	sensor: {
		...device.Serial.default,
		transmit: config.transmit,
		receive: config.receive,
		port: config.port
	}
});

trace(`Sensor identification is: ${JSON.stringify(sensor.identification)}\n`);
Timer.set(() => {
	sensor.sample(onSample);
}, 0, 10_000);

function onSample(error, sample) {
	if (error) {
		trace(`Error in Serial Read: ${error.message}\n`);
	} else {
		trace(`Sample at ${new Date().toLocaleTimeString()}: \n`);
		trace(`\tVoltage:\t\t${sample.voltage} V\n`);
		trace(`\tCurrent:\t\t${sample.current} A\n`);
		trace(`\tPower:\t\t\t${sample.power} W\n`);
		trace(`\tEnergy:\t\t\t${sample.energy} Wh\n`);
		trace(`\tFrequency:\t\t${sample.frequency} Hz\n`);
		trace(`\tPower Factor:\t${sample.powerFactor}\n`);
		trace(`\tIn Alarm?\t\t${sample.alarmStatus ? "Yes" : "No"}\n\n`);
	}
}