/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

/*
 * Test of SIM7100 with multiple functionality.
 * Enable GPS, take a reading, send it in a SMS message.
 */

import Timer from "timer";
import SIM7100 from "sim7100";

const PHONE = "---Phone number here---";
const MESSAGE = "Hello Werld!";

let sim7100 = new SIM7100({ "timeout": 10000 });

sim7100.onReady = function() {
	sim7100.enableGPS();
}

sim7100.onStopped = function() {
	trace("SIM7100 stopped.\n");
}

sim7100.onGPSEnabled = function (device) {
	trace("GPS enabled. Get reading.\n");
	device.getGPS();
}

sim7100.onGPSChanged = function(gps) {
	trace(`${gps.seq} GPS: lat:${gps.lat}, lon:${gps.lon}\n`);
	trace(`  ${gps.date.month}/${gps.date.day}/${gps.date.year} @ ${gps.time.hour}:${gps.time.min}:${gps.time.sec}\n`);
	trace(`  Alt: ${gps.alt} m  -- Speed: ${gps.speed} -- Course: ${gps.course}˚\n`);
	sim7100.disableGPS();
}

sim7100.onGPSDisabled = function (device) {
	trace("GPS disabled. Send a text.\n");
	device.sendSMS(PHONE, MESSAGE + ` lat: ${device.gps.lat} lon: ${device.gps.lon}`);
}

sim7100.onSMSSendComplete = function(device) {
	trace("SMS sent! - stopping sim7100\n");
	sim7100.stop();
}

sim7100.start();


